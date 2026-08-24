from xplpc.core.xplpc import XPLPC
from xplpc.message.param import Param


class Request:
    def __init__(self, function_name: str, params: list[Param] | None = None):
        self.function_name = function_name
        self.params = params

    def data(self) -> str:
        return XPLPC().config.serializer.encode_request(
            self.function_name,
            self.params,
        )
