from ctypes import Array, addressof, c_char
from typing import Any

from xplpc.util.log import Log


class DataView:
    def __init__(self, ptr: int = 0, size: int = 0):
        self.ptr = ptr
        self.size = size
        self._storage: Array[c_char] | None = None

    # The view holds the shared buffer, so the source cannot move or release it while the address is in use.
    @staticmethod
    def create_from_byte_buffer(data, size: int) -> "DataView":
        storage = (c_char * size).from_buffer(data)

        view = DataView(addressof(storage), size)
        view._storage = storage

        return view

    def to_json(self) -> dict[str, Any]:
        return {
            "ptr": self.ptr,
            "size": self.size,
        }

    # This is how a mapping turns the value it received into a view, so what the wire carried decides the answer rather than the caller.
    @classmethod
    def from_json(cls, data: dict[str, Any]) -> "DataView":
        ptr = data.get("ptr") if isinstance(data, dict) else None
        size = data.get("size") if isinstance(data, dict) else None

        # A bool is an int here, and it would reach the helper as address one rather than as no address at all.
        if (
            not isinstance(ptr, int)
            or not isinstance(size, int)
            or isinstance(ptr, bool)
            or isinstance(size, bool)
        ):
            Log.e("[DataView : from_json] Value is not a data view")
            Log.d(
                f"[DataView : from_json] Value is not a data view: {type(data).__name__}"
            )

            return cls()

        return cls(ptr, size)
