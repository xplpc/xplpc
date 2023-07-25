from xplpc.core.config import Config


class SingletonMeta(type):
    _instances = {}

    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(SingletonMeta, cls).__call__(*args, **kwargs)
        return cls._instances[cls]


class XPLPC(metaclass=SingletonMeta):
    def __init__(self):
        self.initialized = False
        self.config = None

    def initialize(self, config: Config):
        if self.initialized:
            return

        self.initialized = True
        self.config = config

    def is_initialized(self):
        return self.initialized
