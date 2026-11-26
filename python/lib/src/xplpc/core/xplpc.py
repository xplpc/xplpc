import threading

from xplpc.core.config import Config
from xplpc.layer.layer import Layer
from xplpc.util.singleton import Singleton


class XPLPC(Singleton):
    def _init(self):
        self.lock = threading.Lock()
        self.initialized = False
        self.config = None

    def initialize(self, config: Config):
        with self.lock:
            if self.initialized:
                return

            self.config = config

            Layer().initialize()

            # This is only set once the native library is bound, since a failure above must not leave a half built singleton.
            self.initialized = True
