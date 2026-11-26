import threading
from typing import Any


class Singleton:
    # Each subclass is its own singleton, and an instance is published only once it is fully built, so a second thread never receives one whose fields are not there yet.

    _instance: Any = None
    _instance_lock: threading.Lock

    def __init_subclass__(cls, **kwargs):
        super().__init_subclass__(**kwargs)

        cls._instance = None
        cls._instance_lock = threading.Lock()

    def __new__(cls):
        instance = cls._instance

        if instance is not None:
            return instance

        with cls._instance_lock:
            if cls._instance is None:
                instance = super().__new__(cls)
                instance._init()
                cls._instance = instance

            return cls._instance

    def _init(self) -> None:
        pass
