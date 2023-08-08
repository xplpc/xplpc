import asyncio
import logging as log
from concurrent.futures import ThreadPoolExecutor
from threading import Thread

from xplpc.core.xplpc import XPLPC
from xplpc.data.callback_list import CallbackList
from xplpc.message.request import Request
from xplpc.proxy.platform_proxy import PlatformProxy
from xplpc.util.unique_id import UniqueID


class Client:
    executor = ThreadPoolExecutor(max_workers=4)

    class SyncCall:
        def __init__(self, request: Request, class_type=None):
            self.request = request
            self.class_type = class_type
            self.key = UniqueID().generate()
            self.response_data = None
            self.make_call()

        def make_call(self):
            def callback(response):
                try:
                    self.response_data = (
                        XPLPC().config.serializer.decode_function_return_value(
                            response, self.class_type
                        )
                    )
                except Exception as e:
                    log.error(f"[Client : call] Error: {e}")

            CallbackList().add(self.key, callback)
            PlatformProxy().native_call_proxy(self.key, self.request.data())

        def run(self):
            return self.response_data

    class SyncCallFromString:
        def __init__(self, request_data: str):
            self.request_data = request_data
            self.key = UniqueID().generate()
            self.response_data = None
            self.make_call()

        def make_call(self):
            def callback(response):
                try:
                    self.response_data = response
                except Exception as e:
                    log.error(f"[Client : call_from_string] Error: {e}")

            CallbackList().add(self.key, callback)
            PlatformProxy().native_call_proxy(self.key, self.request_data)

        def run(self):
            return self.response_data

    class AsyncCall:
        def __init__(self, request: Request, class_type=None, loop=None):
            self.request = request
            self.class_type = class_type
            self.loop = loop if loop else asyncio.get_event_loop()
            self.key = UniqueID().generate()
            self.future = self.loop.create_future()
            self.make_call()

        def make_call(self):
            def callback(response):
                try:
                    self.response = (
                        XPLPC().config.serializer.decode_function_return_value(
                            response, self.class_type
                        )
                    )
                    if not self.future.done():
                        self.loop.call_soon_threadsafe(
                            self.future.set_result, self.response
                        )
                except Exception as e:
                    log.error(f"[Client : async_call] Error: {e}")
                    if not self.future.done():
                        self.loop.call_soon_threadsafe(self.future.set_exception, e)

            try:
                CallbackList().add(self.key, callback)
                self.loop.run_in_executor(
                    Client.executor,
                    PlatformProxy().native_call_proxy,
                    self.key,
                    self.request.data(),
                )
            except Exception as e:
                log.error(f"[Client : async_call] Error: {e}")
                if not self.future.done():
                    self.loop.call_soon_threadsafe(self.future.set_exception, e)

        async def __aenter__(self):
            await self.future
            return self.response

        async def __aexit__(self, exc_type, exc_val, exc_tb):
            pass

    class AsyncCallFromString:
        def __init__(self, request_data: str, loop=None):
            self.request_data = request_data
            self.loop = loop if loop else asyncio.get_event_loop()
            self.key = UniqueID().generate()
            self.future = self.loop.create_future()
            self.make_call()

        def make_call(self):
            def callback(response):
                try:
                    self.response = response
                    if not self.future.done():
                        self.loop.call_soon_threadsafe(
                            self.future.set_result, self.response
                        )
                except Exception as e:
                    log.error(f"[Client : async_call_from_string] Error: {e}")
                    if not self.future.done():
                        self.loop.call_soon_threadsafe(self.future.set_exception, e)

            try:
                CallbackList().add(self.key, callback)
                self.loop.run_in_executor(
                    Client.executor,
                    PlatformProxy().native_call_proxy,
                    self.key,
                    self.request_data,
                )
            except Exception as e:
                log.error(f"[Client : async_call_from_string] Error: {e}")
                if not self.future.done():
                    self.loop.call_soon_threadsafe(self.future.set_exception, e)

        async def __aenter__(self):
            await self.future
            return self.response

        async def __aexit__(self, exc_type, exc_val, exc_tb):
            pass

    class ThreadCall:
        def __init__(self, request: Request, class_type=None, callback=None):
            self.request = request
            self.class_type = class_type
            self.key = UniqueID().generate()
            self.callback = callback
            Thread(target=self.make_call).start()

        def make_call(self):
            def internal_callback(response):
                try:
                    result = XPLPC().config.serializer.decode_function_return_value(
                        response, self.class_type
                    )
                    if self.callback:
                        self.callback(result)
                except Exception as e:
                    log.error(f"[Client : thread_call] Error: {e}")
                    if self.callback:
                        self.callback(None, e)

            CallbackList().add(self.key, internal_callback)
            PlatformProxy().native_call_proxy(self.key, self.request.data())

    class ThreadCallFromString:
        def __init__(self, request_data: str, callback=None):
            self.request_data = request_data
            self.key = UniqueID().generate()
            self.callback = callback
            Thread(target=self.make_call).start()

        def make_call(self):
            def internal_callback(response):
                try:
                    if self.callback:
                        self.callback(response)
                except Exception as e:
                    log.error(f"[Client : thread_call_from_string] Error: {e}")
                    if self.callback:
                        self.callback(None, e)

            CallbackList().add(self.key, internal_callback)
            PlatformProxy().native_call_proxy(self.key, self.request_data)

    @staticmethod
    def call(request: Request, class_type=None):
        return Client.SyncCall(request, class_type).run()

    @staticmethod
    def call_from_string(request_data: str):
        return Client.SyncCallFromString(request_data).run()

    @staticmethod
    async def async_call(request: Request, class_type=None, loop=None):
        async_call_instance = Client.AsyncCall(request, class_type, loop)
        async with async_call_instance as response:
            return response

    @staticmethod
    async def async_call_from_string(request_data: str, loop=None):
        async_call_instance = Client.AsyncCallFromString(request_data, loop)
        async with async_call_instance as response:
            return response

    @staticmethod
    def thread_call(request: Request, class_type=None, callback=None):
        Client.ThreadCall(request, class_type, callback)

    @staticmethod
    def thread_call_from_string(request_data: str, callback=None):
        Client.ThreadCallFromString(request_data, callback)
