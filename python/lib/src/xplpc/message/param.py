from typing import Any


class Param:
    def __init__(self, name: str, value: Any):
        self.name = name
        self.value = value

    def to_json(self) -> dict[str, Any]:
        return {
            "n": self.name,
            "v": self.value,
        }
