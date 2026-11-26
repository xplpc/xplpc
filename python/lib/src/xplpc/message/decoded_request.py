from xplpc.message.message import Message


class DecodedRequest:
    def __init__(self, function_name: str, message: Message):
        self.function_name = function_name
        self.message = message
