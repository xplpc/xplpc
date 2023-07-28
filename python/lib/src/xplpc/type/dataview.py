from ctypes import addressof, c_char
from typing import Any, Dict


class DataView:
    def __init__(self, ptr=None, size=None):
        if ptr is None and size is None:
            self.ptr = 0
            self.size = 0
        elif ptr is not None and size is not None:
            self.ptr = ptr
            self.size = size
        else:
            raise ValueError(
                "Both ptr and size must be provided, or both should be omitted"
            )

    @staticmethod
    def create_from_byte_buffer(data, size):
        # create a buffer from the data
        Buffer = c_char * size
        buf = Buffer.from_buffer(data)

        return DataView(addressof(buf), size)

    def to_json(self) -> Dict[str, Any]:
        return {
            "ptr": self.ptr,
            "size": self.size,
        }

    def from_json(cls, data: Dict[str, Any]) -> "DataView":
        return cls(data["ptr"], data["size"])
