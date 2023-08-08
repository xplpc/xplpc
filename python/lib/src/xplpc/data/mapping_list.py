import threading
from typing import Optional

from xplpc.map.mapping_item import MappingItem


class MappingList:
    # singleton
    _instance = None
    _lock = threading.Lock()

    def __new__(cls):
        if cls._instance is None:
            with cls._lock:
                if cls._instance is None:
                    cls._instance = super(MappingList, cls).__new__(cls)
                    cls._instance._init()
        return cls._instance

    def _init(self):
        # properties
        self.list = {}

    # methods
    def add(self, name: str, item: MappingItem):
        with self._lock:
            self.list[name] = item

    def find(self, name: str) -> Optional[MappingItem]:
        with self._lock:
            return self.list.get(name)

    def has(self, name: str) -> bool:
        with self._lock:
            return name in self.list

    def clear(self):
        with self._lock:
            self.list.clear()
