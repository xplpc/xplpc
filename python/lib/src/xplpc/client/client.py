import logging as log

from xplpc.core.xplpc import XPLPC
from xplpc.data.callback_list import CallbackList
from xplpc.message.request import Request
from xplpc.proxy.platform_proxy import PlatformProxy
from xplpc.type import typedefs
from xplpc.util.unique_id import UniqueID


class Client:
    @staticmethod
    def call(
        request: Request,
        callback: typedefs.ClientCallback,
    ):
        try:
            key = UniqueID().generate()

            CallbackList().add(
                key,
                lambda response: callback(
                    XPLPC().config.serializer.decode_function_return_value(response)
                ),
            )

            PlatformProxy().native_call_proxy(key, request.data())
        except Exception as e:
            log.error(f"[Client : call] Error: {e}")
            callback(None)

    @staticmethod
    def call_from_string(
        request_data: str,
        callback: typedefs.ClientCallbackFromString,
    ):
        try:
            key = UniqueID().generate()

            CallbackList().add(key, lambda response: callback(response))

            PlatformProxy().native_call_proxy(key, request_data)
        except Exception as e:
            log.error(f"[Client : call] Error: {e}")
            callback("")
