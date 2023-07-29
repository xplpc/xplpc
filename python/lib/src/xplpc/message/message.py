from typing import Any, Optional


class Message:
    def __init__(self):
        self.data = {}

    def get(self, name: str) -> Optional[Any]:
        return self.data.get(name)

    def set(self, name: str, value: Any):
        self.data[name] = value
