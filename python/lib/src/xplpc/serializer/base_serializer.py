class BaseSerializer:
    def decode_function_name(self, data):
        pass

    def decode_function_return_value(self, data, class_type=None):
        pass

    def decode_message(self, data):
        pass

    def encode_function_return_value(self, data):
        pass

    def encode_request(self, function_name, params=None):
        pass
