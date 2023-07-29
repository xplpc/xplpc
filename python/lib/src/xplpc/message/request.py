from typing import List, Optional

from xplpc.core.xplpc import XPLPC
from xplpc.message.param import Param


class Request:
    def __init__(self, function_name: str, params: Optional[List[Param]] = None):
        self.functionName = function_name
        self.params = params

    def data(self) -> str:
        return XPLPC().config.serializer.encode_request(
            self.functionName,
            self.params,
        )
