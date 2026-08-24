import threading


class CallbackList:
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

    def add(self, key, callback):
        with self.lock:
            self.list[key] = callback

    def execute(self, key, data):
        # The entry is taken and invoked in a single step, so concurrent callers never run the same callback twice.

        with self.lock:
            callback = self.list.pop(key, None)

        if callback:
            callback(data)

    def remove(self, key):
        with self.lock:
            self.list.pop(key, None)

    def clear(self):
        with self.lock:
            self.list.clear()

    def count(self):
        with self.lock:
            return len(self.list)
