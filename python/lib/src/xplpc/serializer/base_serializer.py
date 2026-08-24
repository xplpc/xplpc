from abc import ABC, abstractmethod
from typing import Any

from xplpc.message.decoded_request import DecodedRequest
from xplpc.message.param import Param


class BaseSerializer(ABC):
    @abstractmethod
    def decode_function_return_value(
        self, data: str, class_type=None
    ) -> Any | None: ...

    @abstractmethod
    def decode_request(self, data: str) -> DecodedRequest | None: ...

    @abstractmethod
    def encode_function_return_value(self, data: Any) -> str: ...

    @abstractmethod
    def encode_request(
        self, function_name: str, params: list[Param] | None = None
    ) -> str: ...
