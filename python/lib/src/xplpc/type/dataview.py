from typing import Any

from xplpc.layer.layer import Layer
from xplpc.util.log import Log

# A handle is a 64 bit unsigned integer, and ctypes wraps anything outside that range into another handle on its way to the native side.
HANDLE_LIMIT = 2**64


class DataView:
    # A view names a buffer its owner registered, which is all that crosses the wire, so a reader can never be pointed at memory nobody registered.

    def __init__(self, buffer: int = 0):
        self.buffer = buffer
        self._registered = False

    # The layer holds the source until the view is released, so the bytes cannot move or be freed while a reader can reach them, and a view never released keeps its source for good.
    @staticmethod
    def create_from_byte_buffer(source) -> "DataView":
        view = DataView(Layer().register_buffer(source))
        view._registered = True

        return view

    # Only the view that registered a buffer can release it, so releasing a view this side received, even one naming a buffer it registered, or one already released, answers false.
    def release(self) -> bool:
        if not self._registered:
            return False

        self._registered = False

        return Layer().release_buffer(self.buffer)

    def to_json(self) -> dict[str, Any]:
        return {"buffer": self.buffer}

    # This is how a mapping turns the value it received into a view, so what the wire carried decides the answer rather than the caller.
    @classmethod
    def from_json(cls, data: dict[str, Any]) -> "DataView":
        buffer = data.get("buffer") if isinstance(data, dict) else None

        # A bool is an int here, and it would reach the native side as handle one rather than as no handle at all.
        if (
            not isinstance(buffer, int)
            or isinstance(buffer, bool)
            or not 0 <= buffer < HANDLE_LIMIT
        ):
            Log.e("[DataView : from_json] Value is not a data view")
            Log.d(
                f"[DataView : from_json] Value is not a data view: {type(data).__name__}"
            )

            return cls()

        return cls(buffer)
