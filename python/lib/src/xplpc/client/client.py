import asyncio
import threading

from xplpc.core.xplpc import XPLPC
from xplpc.layer.layer import Layer, PendingCall
from xplpc.message.request import Request
from xplpc.type.typedefs import ClientCallback, ClientCallbackFromString
from xplpc.util.log import Log


class _LoopAnswer:
    # An answer is produced on whatever thread the mapping chose, and the future it resolves belongs to the loop that is waiting, which may be cancelled or closed by then.

    def __init__(self, loop: asyncio.AbstractEventLoop, source: str):
        self._loop = loop
        self._future = loop.create_future()
        self._source = source
        self._cancelled = threading.Event()

    async def wait(self, pending: PendingCall):
        try:
            return await self._future
        except asyncio.CancelledError:
            # The caller is gone, so the call is taken back and an answer that wins the race is dropped here.
            self._cancelled.set()
            Layer().cancel(pending)
            raise

    def deliver(self, value) -> None:
        if self._cancelled.is_set():
            return

        # Closing a loop is not something this side can observe atomically with scheduling on it, so a loop that closed in between is reported rather than raised inside a native frame.
        try:
            self._loop.call_soon_threadsafe(self._settle, value)
        except RuntimeError:
            Log.e(
                f"[Client : {self._source}] The answer arrived after its loop was closed"
            )

    def _settle(self, value) -> None:
        # A future belongs to its loop, so whether it is still waiting is read there and nowhere else.

        if not self._future.done():
            self._future.set_result(value)


class Client:
    # Reading the config or the native entry points before there are any fails on whatever happens to be missing, which names nothing a caller can act on.
    @staticmethod
    def _require_initialized(source: str) -> None:
        if XPLPC().initialized:
            return

        Log.e(f"[Client : {source}] XPLPC was not initialized")

        raise RuntimeError("XPLPC was not initialized")

    @staticmethod
    def call(
        request: Request, callback: ClientCallback | None = None, class_type=None
    ) -> None:
        Client._require_initialized("call")

        def answer(response):
            value = Client._decode(response, class_type, "call")

            if callback:
                callback(value)

        Client._dispatch(PendingCall(answer), Client._data(request, "call"), "call")

    @staticmethod
    def call_from_string(
        request_data: str, callback: ClientCallbackFromString | None = None
    ) -> None:
        Client._require_initialized("call_from_string")

        def answer(response):
            if callback:
                callback(response)

        Client._dispatch(PendingCall(answer), request_data, "call_from_string")

    @staticmethod
    def call_sync(request: Request, class_type=None):
        Client._require_initialized("call_sync")

        response = Client._answer_synchronously(
            Client._data(request, "call_sync"), "call_sync"
        )

        if response is None:
            return None

        return Client._decode(response, class_type, "call_sync")

    @staticmethod
    def call_sync_from_string(request_data: str) -> str:
        Client._require_initialized("call_sync_from_string")

        response = Client._answer_synchronously(request_data, "call_sync_from_string")

        return response if response is not None else ""

    @staticmethod
    async def call_async(request: Request, class_type=None):
        Client._require_initialized("call_async")

        waiter = _LoopAnswer(asyncio.get_running_loop(), "call_async")

        def answer(response):
            waiter.deliver(Client._decode(response, class_type, "call_async"))

        pending = PendingCall(answer)
        Client._dispatch(pending, Client._data(request, "call_async"), "call_async")

        return await waiter.wait(pending)

    @staticmethod
    async def call_async_from_string(request_data: str) -> str:
        Client._require_initialized("call_async_from_string")

        waiter = _LoopAnswer(asyncio.get_running_loop(), "call_async_from_string")

        pending = PendingCall(waiter.deliver)
        Client._dispatch(pending, request_data, "call_async_from_string")

        return await waiter.wait(pending)

    @staticmethod
    def _decode(response: str, class_type, source: str):
        # Only the decode is guarded, so a callback that raises on a valid answer is never called a second time with nothing.

        try:
            return XPLPC().config.serializer.decode_function_return_value(
                response, class_type
            )
        except Exception as e:
            Log.e(f"[Client : {source}] Error when decode data")
            Log.d(f"[Client : {source}] Error when decode data: {e}")

        return None

    @staticmethod
    def _answer_synchronously(data: str, source: str) -> str | None:
        # A mapping that had not answered when the call returned is cancelled, so its later answer resolves nothing and leaves nothing behind.

        answer: list[str] = []
        pending = PendingCall(answer.append)

        Client._dispatch(pending, data, source)

        if answer:
            return answer[0]

        Layer().cancel(pending)
        Log.e(f"[Client : {source}] The function did not answer synchronously")

        return None

    @staticmethod
    def _data(request: Request, source: str) -> str:
        # A serializer of the caller's own can raise while it writes the request, and the caller is answered rather than handed the exception.

        try:
            return request.data()
        except Exception as e:
            Log.e(f"[Client : {source}] Error when encode the request")
            Log.d(f"[Client : {source}] Error when encode the request: {e}")

            return ""

    @staticmethod
    def _dispatch(pending: PendingCall, data: str, source: str) -> None:
        try:
            Layer().call(data, pending)
        except UnicodeEncodeError as e:
            # Text the wire cannot carry is a failing path like any other, and a caller reading a path off the filesystem produces it.
            Log.e(f"[Client : {source}] Error when encode the request")
            Log.d(f"[Client : {source}] Error when encode the request: {e}")
            pending.callback("")
