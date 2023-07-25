from typing import Any, Dict

from xplpc.message.param import Param


class Param:
    def __init__(self, name: str, value: Any):
        self.name = name
        self.value = value

    def to_json(self) -> Dict[str, Any]:
        return {
            "n": self.name,
            "v": self.value,
        }

    @classmethod
    def from_json(cls, json_data: Dict[str, Any]) -> Param:
        return cls(json_data["n"], json_data["v"])
