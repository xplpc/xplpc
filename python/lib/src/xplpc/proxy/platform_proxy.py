import json
import logging as log
import os
import platform
import sys
from ctypes import CFUNCTYPE, c_bool, c_char_p, c_size_t, cdll

import xplpc.util.plaform as p
from xplpc.data.callback_list import CallbackList
from xplpc.data.mapping_list import MappingList


class PlatformProxy:
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super(PlatformProxy, cls).__new__(cls)
            cls._instance._init()
        return cls._instance

    def _init(self):
        self.libc = None
        self.xplpc_core_initialize = None
        self.xplpc_core_finalize = None
        self.xplpc_core_is_initialized = None
        self.xplpc_native_call_proxy = None
        self.xplpc_native_call_proxy_callback = None

        self.initialize_callback = None
        self.finalize_callback = None
        self.has_mapping_callback = None
        self.native_proxy_call_callback = None
        self.native_proxy_callback_callback = None

    def initialize(self):
        # load the shared library into c types
        log.debug("Loading native library...")

        lib_path = self.get_lib_path()
        self.libc = cdll.LoadLibrary(lib_path)

        log.debug("OK")

        # declare the prototypes for the functions.
        OnInitializePlatform = CFUNCTYPE(None)
        OnFinalizePlatform = CFUNCTYPE(None)
        OnHasMapping = CFUNCTYPE(c_bool, c_char_p, c_size_t)
        OnNativeProxyCallback = CFUNCTYPE(None, c_char_p, c_size_t, c_char_p, c_size_t)
        OnNativeProxyCall = CFUNCTYPE(None, c_char_p, c_size_t, c_char_p, c_size_t)

        log.debug("Binding xplpc_core_initialize...")
        self.xplpc_core_initialize = self.libc.xplpc_core_initialize
        self.xplpc_core_initialize.argtypes = [
            c_bool,
            OnInitializePlatform,
            OnFinalizePlatform,
            OnHasMapping,
            OnNativeProxyCall,
            OnNativeProxyCallback,
        ]

        log.debug("Binding xplpc_core_finalize...")
        self.xplpc_core_finalize = self.libc.xplpc_core_finalize

        log.debug("Binding xplpc_core_is_initialized...")
        self.xplpc_core_is_initialized = self.libc.xplpc_core_is_initialized

        log.debug("Binding xplpc_native_call_proxy...")
        self.xplpc_native_call_proxy = self.libc.xplpc_native_call_proxy
        self.xplpc_native_call_proxy.argtypes = [
            c_char_p,
            c_size_t,
            c_char_p,
            c_size_t,
        ]

        log.debug("Binding xplpc_native_call_proxy_callback...")
        self.xplpc_native_call_proxy_callback = (
            self.libc.xplpc_native_call_proxy_callback
        )
        self.xplpc_native_call_proxy_callback.argtypes = [
            c_char_p,
            c_size_t,
            c_char_p,
            c_size_t,
        ]

        log.debug("Binding with Python...")
        self.initialize_callback = OnInitializePlatform(self.onInitializePlatform)
        self.finalize_callback = OnFinalizePlatform(self.onFinalizePlatform)
        self.has_mapping_callback = OnHasMapping(self.onHasMapping)
        self.native_proxy_call_callback = OnNativeProxyCall(self.onNativeProxyCall)
        self.native_proxy_callback_callback = OnNativeProxyCallback(
            self.onNativeProxyCallback
        )

        log.debug("Calling xplpc_core_initialize...")
        self.xplpc_core_initialize(
            True,
            self.initialize_callback,
            self.finalize_callback,
            self.has_mapping_callback,
            self.native_proxy_call_callback,
            self.native_proxy_callback_callback,
        )

        log.debug("Calling xplpc_core_is_initialized...")
        is_init = self.xplpc_core_is_initialized()
        log.debug(f"Is initialized? {is_init}")

    def onInitializePlatform(self):
        pass

    def onFinalizePlatform(self):
        MappingList().clear()

    def onHasMapping(self, name, nameSize):
        return MappingList().has(name[:nameSize].decode())

    def onNativeProxyCall(self, key, keySize, data, dataSize):
        from xplpc.core.xplpc import XPLPC

        # TODO: optimize using only decode for data since key is the same received
        keyStr = key[:keySize].decode()
        dataStr = data[:dataSize].decode()

        # function name
        functionName = XPLPC().config.serializer.decode_function_name(dataStr)

        if not functionName:
            log.error("[PlatformProxy : call] Function name is empty")
            self.native_call_proxy_callback(keyStr, "")
            return

        # mapping item
        mappingItem = MappingList().find(functionName)

        if not mappingItem:
            log.error(
                "[PlatformProxy : call] Mapping not found for function: " + functionName
            )

            self.native_call_proxy_callback(keyStr, "")

            return

        # execute
        try:
            message = XPLPC().config.serializer.decode_message(dataStr)
        except Exception as e:
            log.error(f"[PlatformProxy : call] Error when decode message: {e}")
            message = None

        if not message:
            log.error(
                f"[PlatformProxy : call] Error when decode message for function: {functionName}"
            )

            self.native_call_proxy_callback(keyStr, "")

            return

        try:
            mappingItem.target(
                message,
                lambda result: self.native_call_proxy_callback(
                    keyStr,
                    XPLPC().config.serializer.encode_function_return_value(result),
                ),
            )
        except Exception as e:
            log.error(f"[PlatformProxy : call] Error: {e}")
            self.native_call_proxy_callback(keyStr, "")

    def onNativeProxyCallback(self, key, keySize, data, dataSize):
        CallbackList().execute(
            key[:keySize].decode(),
            data[:dataSize].decode(),
        )

    def native_call_proxy_callback(self, key, data):
        # create ctypes string buffer
        key_buffer = key.encode("utf-8")
        data_buffer = data.encode("utf-8")

        # call native function
        self.xplpc_native_call_proxy_callback(
            key_buffer,
            len(key_buffer),
            data_buffer,
            len(data_buffer),
        )

    def native_call_proxy(self, key, data):
        # create ctypes string buffer
        key_buffer = key.encode("utf-8")
        data_buffer = data.encode("utf-8")

        # call native function
        self.xplpc_native_call_proxy(
            key_buffer,
            len(key_buffer),
            data_buffer,
            len(data_buffer),
        )

    def get_lib_path(self):
        # library name
        arch = platform.machine().lower()
        plat = sys.platform.lower()

        lib_arch = ""
        lib_name = ""
        lib_path = ""

        if plat.startswith("win"):
            lib_name = "xplpc.dll"
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
        elif arch == "x86_64" or arch == "amd64":
            lib_arch = "x86_64"
        else:
            raise OSError(f"The architecture {arch} is not supported.")

        # detect lib binary path
        if getattr(sys, "frozen", False):
            # frozen mode
            lib_path = os.path.abspath(
                os.path.join(
                    os.path.dirname(__file__),
                    "..",
                    "..",
                    "xplpc",
                    "lib",
                    lib_arch,
                    lib_name,
                )
            )
        else:
            # detect dev or pip mode
            lib_binary_dir = p.get_lib_binary_dir()

            build_dir = os.path.abspath(
                os.path.join(
                    os.path.dirname(__file__),
                    "..",
                    "..",
                    "..",
                    "..",
                    "..",
                    "build",
                    "c-shared",
                )
            )

            if os.path.exists(build_dir):
                # dev mode
                lib_path = os.path.abspath(
                    os.path.join(
                        build_dir,
                        lib_arch,
                        lib_binary_dir,
                        lib_name,
                    )
                )
            else:
                # pip mode
                lib_path = os.path.abspath(
                    os.path.join(
                        os.path.dirname(__file__),
                        "..",
                        "lib",
                        lib_arch,
                        lib_name,
                    )
                )

        return lib_path
