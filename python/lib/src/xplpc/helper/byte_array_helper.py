from ctypes import c_char


class ByteArrayHelper:
    @staticmethod
    def create_from_data_view(data_view):
        # create ctypes array from dataview's address and size
        ctypes_array = (c_char * data_view.size).from_address(data_view.ptr)

        # create bytearray from ctypes array
        byte_array = bytearray(ctypes_array)

        return byte_array
