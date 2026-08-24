import asyncio
import threading

from xplpc.core.xplpc import XPLPC
from xplpc.data.callback_list import CallbackList
from xplpc.message.request import Request
from xplpc.proxy.platform_proxy import PlatformProxy
from xplpc.type.typedefs import ClientCallback, ClientCallbackFromString
from xplpc.util.log import Log
from xplpc.util.unique_id import UniqueID


class _SyncAnswer:
    def __init__(self):
        self._lock = threading.Lock()
        self._value = None

    def set(self, data: str) -> None:
        with self._lock:
            self._value = data

    def get(self) -> str | None:
        with self._lock:
            return self._value


class Client:
    # Reading the config or the native entry points before there are any fails on whatever happens to be missing, which names nothing a caller can act on.
    @staticmethod
    def _require_initialized(source: str) -> None:
        if XPLPC().initialized:
            return

        Log.e(f"[Client : {source}] The library was not initialized")

        raise RuntimeError("XPLPC was not initialized")

    @staticmethod
    def call(
        request: Request, callback: ClientCallback | None = None, class_type=None
    ) -> None:
        Client._require_initialized("call")

        def answer(response):
            if not callback:
                return

            try:
                callback(
                    XPLPC().config.serializer.decode_function_return_value(
                        response, class_type
                    )
                )
            except Exception as e:
                Log.e("[Client : call] Error when decode data")
                Log.d(f"[Client : call] Error when decode data: {e}")
                callback(None)

        Client._dispatch(UniqueID.generate(), request.data(), answer, "call")

    @staticmethod
    def call_from_string(
        request_data: str, callback: ClientCallbackFromString | None = None
    ) -> None:
        Client._require_initialized("call_from_string")

        def answer(response):
            if callback:
                callback(response)

        Client._dispatch(UniqueID.generate(), request_data, answer, "call_from_string")

    @staticmethod
    def call_sync(request: Request, class_type=None):
        Client._require_initialized("call_sync")

        response = Client._answer_synchronously(request.data(), "call_sync")

        if response is None:
            return None

        try:
            return XPLPC().config.serializer.decode_function_return_value(
                response, class_type
            )
        except Exception as e:
            Log.e("[Client : call_sync] Error when decode data")
            Log.d(f"[Client : call_sync] Error when decode data: {e}")

        return None

    @staticmethod
    def call_sync_from_string(request_data: str) -> str:
        Client._require_initialized("call_sync_from_string")

        response = Client._answer_synchronously(request_data, "call_sync_from_string")

        return response if response is not None else ""

    @staticmethod
    async def call_async(request: Request, class_type=None):
        Client._require_initialized("call_async")

        loop = asyncio.get_running_loop()
        future = loop.create_future()

        def settle(value):
            # A future belongs to its loop, so whether it is still waiting is read there and nowhere else.

            if not future.done():
                future.set_result(value)

        def answer(response):
            try:
                value = XPLPC().config.serializer.decode_function_return_value(
                    response, class_type
                )
            except Exception as e:
                Log.e("[Client : call_async] Error when decode data")
                Log.d(f"[Client : call_async] Error when decode data: {e}")
                value = None

            loop.call_soon_threadsafe(settle, value)

        key = UniqueID.generate()
        Client._dispatch(key, request.data(), answer, "call_async")

        try:
            return await future
        except asyncio.CancelledError:
            # Nothing will ever resolve this key once the caller is gone, and an answer arriving later would reach a loop that may already be closed.
            CallbackList().remove(key)
            raise

    @staticmethod
    async def call_async_from_string(request_data: str) -> str:
        Client._require_initialized("call_async_from_string")

        loop = asyncio.get_running_loop()
        future = loop.create_future()

        def settle(value):
            if not future.done():
                future.set_result(value)

        def answer(response):
            loop.call_soon_threadsafe(settle, response)

        key = UniqueID.generate()
        Client._dispatch(key, request_data, answer, "call_async_from_string")

        try:
            return await future
        except asyncio.CancelledError:
            CallbackList().remove(key)
            raise

    @staticmethod
    def _answer_synchronously(data: str, source: str) -> str | None:
        # A mapping is free to answer from a thread of its own after this function has returned, so what it writes into outlives the frame and is guarded.
        answer = _SyncAnswer()
        key = UniqueID.generate()

        Client._dispatch(key, data, answer.set, source)

        # Taking the key back is what decides the two cases, since a mapping that answered inline has already taken it and one that deferred never will.
        CallbackList().remove(key)

        response = answer.get()

        if response is None:
            Log.e(f"[Client : {source}] The function did not answer synchronously")

        return response

    @staticmethod
    def _dispatch(
        key: str, data: str, callback: ClientCallbackFromString, source: str
    ) -> None:
        # The registration is dropped when the native side cannot be reached, so a failed call does not leave a key behind forever.

        CallbackList().add(key, callback)

        try:
            PlatformProxy().call_native_proxy(key, data)
        except UnicodeEncodeError as e:
            # Text the wire cannot carry is a failing path like any other, and a caller reading a path off the filesystem produces it.
            Log.e(f"[Client : {source}] Error when encode the request")
            Log.d(f"[Client : {source}] Error when encode the request: {e}")
            CallbackList().execute(key, "")
        except Exception:
            CallbackList().remove(key)
            raise
