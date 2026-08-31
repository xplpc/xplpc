from xplpc.serializer.base_serializer import BaseSerializer


class Config:
    def __init__(self, serializer: BaseSerializer):
        self.serializer = serializer
