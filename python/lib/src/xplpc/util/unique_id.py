import threading


class UniqueID:
    # singleton
    _instance = None
    lock = threading.Lock()
    counter = 0

    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            cls._instance = super(UniqueID, cls).__new__(cls, *args, **kwargs)
        return cls._instance

    def generate(self):
        with self.lock:
            self.counter += 1
            return f"PY-{self.counter}"
