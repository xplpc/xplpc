from xplpc.layer.layer import Layer


class ByteArrayHelper:
    # A view that names nothing, was released or was never registered answers no bytes, and the bytes it does answer are a copy taken while the owner still held them.
    @staticmethod
    def create_from_data_view(data_view) -> bytearray:
        return bytearray(Layer().read_buffer(data_view.buffer))
