from collections.abc import Callable

from xplpc.message.message import Message
from xplpc.message.response import Response

Target = Callable[[Message, Response], None]


class MappingItem:
    def __init__(self, target: Target):
        self.target = target
