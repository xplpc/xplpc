from typing import Any


class Param:
    def __init__(self, name: str, value: Any):
        self.name = name
        self.value = value

    def to_json(self) -> dict[str, Any]:
        value = self.value.to_json() if hasattr(self.value, "to_json") else self.value
        return {
            "n": self.name,
            "v": value,
        }
