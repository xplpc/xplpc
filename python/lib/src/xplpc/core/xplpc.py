import json
import os
import platform
import sys
from ctypes import CFUNCTYPE, c_bool, c_char_p, c_size_t, cdll

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
        if XPLPC.initialized:
            return

        XPLPC.initialized = True
        XPLPC.config = config

        # Load the shared library into c types.

        print("Loading library...")

        # library name
        arch = platform.machine()
        plat = sys.platform
        lib_arch = ""
        lib_name = ""

        if plat.startswith("win"):
            lib_name = "libxplpc.dll"
        elif plat.startswith("darwin"):
            lib_name = "libxplpc.dylib"
        elif plat.startswith("linux"):
            lib_name = "libxplpc.so"
        else:
            raise OSError(f"The platform {plat} is not supported.")

        if arch == "armv7l" or arch == "armv7":
            lib_arch = "arm32"
        elif arch == "aarch64" or arch == "arm64":
            lib_arch = "arm64"
        elif arch == "i686" or arch == "x86":
            lib_arch = "x86"
        elif arch == "x86_64":
            lib_arch = "x86_64"
        else:
            raise OSError(f"The architecture {arch} is not supported.")

        lib_path = os.path.join(
            os.path.dirname(__file__), "..", "lib", lib_arch, lib_name
        )
        libc = cdll.LoadLibrary(lib_path)

        print("OK")

        # Declare the prototypes for the functions.

        OnInitializePlatform = CFUNCTYPE(None)
        OnFinalizePlatform = CFUNCTYPE(None)
        OnHasMapping = CFUNCTYPE(c_bool, c_char_p, c_size_t)
        OnNativeProxyCallback = CFUNCTYPE(None, c_char_p, c_size_t, c_char_p, c_size_t)
        OnNativeProxyCall = CFUNCTYPE(None, c_char_p, c_size_t, c_char_p, c_size_t)

        print("Binding: xplpc_core_initialize...")
        xplpc_core_initialize = libc.xplpc_core_initialize
        xplpc_core_initialize.argtypes = [
            c_bool,
            OnInitializePlatform,
            OnFinalizePlatform,
            OnHasMapping,
            OnNativeProxyCall,
            OnNativeProxyCallback,
        ]
        print("OK")

        print("Binding: xplpc_core_finalize...")
        xplpc_core_finalize = libc.xplpc_core_finalize
        print("OK")

        print("Binding: xplpc_core_is_initialized...")
        xplpc_core_is_initialized = libc.xplpc_core_is_initialized
        print("OK")

        print("Binding: xplpc_native_call_proxy...")
        xplpc_native_call_proxy = libc.xplpc_native_call_proxy
        xplpc_native_call_proxy.argtypes = [c_char_p, c_size_t, c_char_p, c_size_t]
        print("OK")

        print("Binding: xplpc_native_call_proxy_callback...")
        xplpc_native_call_proxy_callback = libc.xplpc_native_call_proxy_callback
        xplpc_native_call_proxy_callback.argtypes = [
            c_char_p,
            c_size_t,
            c_char_p,
            c_size_t,
        ]
        print("OK")

        # Sample

        def my_initialize():
            print("Platform initialized")

        def my_finalize():
            print("Platform finalized")

        def my_has_mapping(name, size):
            print(f"Checking mapping: {name}, {size}")
            return False

        def my_native_proxy_call(key, size, data, dataSize):
            print(f"Native proxy call: {key}, {size}, {data}, {dataSize}")

        def my_native_proxy_callback(key, size, data, dataSize):
            print(f"Native proxy callback: {key}, {size}, {data}, {dataSize}")

        print("Binding with Python...")
        initialize_callback = OnInitializePlatform(my_initialize)
        finalize_callback = OnFinalizePlatform(my_finalize)
        has_mapping_callback = OnHasMapping(my_has_mapping)
        native_proxy_call_callback = OnNativeProxyCall(my_native_proxy_call)
        native_proxy_callback_callback = OnNativeProxyCallback(my_native_proxy_callback)
        print("OK")

        print("Calling xplpc_core_initialize...")
        xplpc_core_initialize(
            True,
            initialize_callback,
            finalize_callback,
            has_mapping_callback,
            native_proxy_call_callback,
            native_proxy_callback_callback,
        )
        print("OK")

        print("Calling xplpc_core_is_initialized...")
        is_init = xplpc_core_is_initialized()
        print(f"Is initialized? {is_init}")
        print("OK")

        print("Calling xplpc_native_call_proxy...")
        data = {"f": "sample.login"}
        data = json.dumps(data).encode("utf-8")
        xplpc_native_call_proxy(b"key", len(b"key"), data, len(data))
        print("OK")

        print("Calling xplpc_native_call_proxy...")
        xplpc_native_call_proxy_callback(b"key", len(b"key"), b"data", len(b"data"))
        print("OK")

        print("Calling xplpc_core_finalize...")
        xplpc_core_finalize()
        print("OK")
