from xplpc.serializer.serializer import Serializer


class Config:
    def __init__(self, serializer: Serializer):
        self.serializer = serializer
