from typing import Any, Callable

from xplpc.message.message import Message

Target = Callable[[Message, Any], None]


class MappingItem:
    def __init__(self, target: Target):
        self.target = target
