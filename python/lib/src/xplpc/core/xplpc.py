from xplpc.core.config import Config
from xplpc.proxy.platform_proxy import PlatformProxy


class XPLPC:
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super(XPLPC, cls).__new__(cls)
            cls._instance._init()
        return cls._instance

    def _init(self):
        self.initialized = False
        self.config = None

    def initialize(self, config: Config):
        if self.initialized:
            return

        self.initialized = True
        self.config = config

        PlatformProxy().initialize()

    def is_initialized(self):
        return self.initialized
