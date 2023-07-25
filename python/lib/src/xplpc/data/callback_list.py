import threading


class CallbackList:
    # singleton
    _instance = None
    _lock = threading.Lock()

    def __new__(cls):
        with cls._lock:
            if cls._instance is None:
                cls._instance = super(CallbackList, cls).__new__(cls)
                cls._instance._init()
        return cls._instance

    def _init(self):
        # properties
        self.list = {}
        self.lock = threading.Lock()

    # methods
    def add(self, key, callback):
        with self.lock:
            self.list[key] = callback

    def execute(self, key, data):
        with self.lock:
            callback = self.list.pop(key, None)

        if callback:
            callback(data)

    def count(self):
        with self.lock:
            return len(self.list)
