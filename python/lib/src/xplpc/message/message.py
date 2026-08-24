from typing import Any


class Message:
    def __init__(self):
        self.data = {}

    def get(self, name: str) -> Any | None:
        return self.data.get(name)

    def set(self, name: str, value: Any):
        self.data[name] = value
