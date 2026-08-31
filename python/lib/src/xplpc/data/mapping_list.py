import threading

from xplpc.map.mapping_item import MappingItem


class MappingList:
    _instance = None
    _instance_lock = threading.Lock()

    def __new__(cls):
        if cls._instance is None:
            with cls._instance_lock:
                if cls._instance is None:
                    cls._instance = super().__new__(cls)
                    cls._instance._init()
        return cls._instance

    def _init(self):
        self.list = {}
        self.lock = threading.Lock()

    def add(self, name: str, item: MappingItem):
        from xplpc.proxy.platform_proxy import PlatformProxy

        # The native side routes from the names it was told, so a failure to declare one must leave nothing behind to be unreachable later.
        PlatformProxy().add_mapping(name)

        with self.lock:
            self.list[name] = item

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
        from xplpc.proxy.platform_proxy import PlatformProxy

        PlatformProxy().clear_mappings()

        with self.lock:
            self.list.clear()
