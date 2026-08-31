from ctypes import c_char


class ByteArrayHelper:
    # Reading from a null address would take the process down, and an empty view carries no bytes.
    @staticmethod
    def create_from_data_view(data_view) -> bytearray:
        if not data_view.ptr or data_view.size <= 0:
            return bytearray()

        return bytearray((c_char * data_view.size).from_address(data_view.ptr))
