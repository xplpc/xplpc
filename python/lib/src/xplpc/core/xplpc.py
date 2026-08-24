import threading

from xplpc.core.config import Config
from xplpc.proxy.platform_proxy import PlatformProxy


class XPLPC:
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
        self.lock = threading.Lock()
        self.initialized = False
        self.config = None

    def initialize(self, config: Config):
        with self.lock:
            if self.initialized:
                return

            self.config = config

            PlatformProxy().initialize()

            # This is only set once the native library is bound, since a failure above must not leave a half built singleton.
            self.initialized = True

    def is_initialized(self):
        return self.initialized
