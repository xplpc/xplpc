import threading

from xplpc.map.mapping_item import MappingItem
from xplpc.util.singleton import Singleton


class MappingList(Singleton):
    def _init(self):
        self.list = {}
        self.lock = threading.Lock()

        # Adding and clearing each change the local list and the names the router holds, so one has to finish both before the other starts.
        self.operations = threading.Lock()

    def add(self, name: str, item: MappingItem):
        from xplpc.layer.layer import Layer

        # The router can send a call the moment it hears a name, so the mapping is recorded first and taken back when declaring it fails, since a name the router never heard is unreachable.

        with self.operations:
            with self.lock:
                previous = self.list.get(name)
                self.list[name] = item

            try:
                Layer().declare(name)
            except BaseException:
                self._restore(name, item, previous)
                raise

    def find(self, name: str) -> MappingItem | None:
        with self.lock:
            return self.list.get(name)

    def has(self, name: str) -> bool:
        with self.lock:
            return name in self.list

    def count(self) -> int:
        with self.lock:
            return len(self.list)

    def clear(self):
        from xplpc.layer.layer import Layer

        # The router forgets the names before the mappings behind them go, so it never sends a call to one that is already gone.
        with self.operations:
            with self.lock:
                names = list(self.list)

            for name in names:
                Layer().retract(name)

            with self.lock:
                self.list.clear()

    def _restore(self, name: str, item: MappingItem, previous: MappingItem | None):
        with self.lock:
            if self.list.get(name) is not item:
                return

            if previous is None:
                del self.list[name]
                return

            self.list[name] = previous
