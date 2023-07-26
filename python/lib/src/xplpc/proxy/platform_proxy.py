import json
import logging as log
import os
import platform
import sys
from ctypes import CFUNCTYPE, c_bool, c_char_p, c_size_t, cdll

from xplpc.data.callback_list import CallbackList
from xplpc.data.mapping_list import MappingList


class PlatformProxy:
    @staticmethod
    def initialize():
        # load the shared library into c types
        log.debug("Loading native library...")

        # library name
        arch = platform.machine().lower()
        plat = sys.platform.lower()
        lib_arch = ""
        lib_name = ""

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

        lib_path = ""

        if getattr(sys, "frozen", False):
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
            lib_path = os.path.abspath(
                os.path.join(
                    os.path.dirname(__file__),
                    "..",
                    "lib",
                    lib_arch,
                    lib_name,
                )
            )

        libc = cdll.LoadLibrary(lib_path)

        log.debug("OK")

        # declare the prototypes for the functions.
        OnInitializePlatform = CFUNCTYPE(None)
        OnFinalizePlatform = CFUNCTYPE(None)
        OnHasMapping = CFUNCTYPE(c_bool, c_char_p, c_size_t)
        OnNativeProxyCallback = CFUNCTYPE(None, c_char_p, c_size_t, c_char_p, c_size_t)
        OnNativeProxyCall = CFUNCTYPE(None, c_char_p, c_size_t, c_char_p, c_size_t)

        log.debug("Binding: xplpc_core_initialize...")
        PlatformProxy.xplpc_core_initialize = libc.xplpc_core_initialize
        PlatformProxy.xplpc_core_initialize.argtypes = [
            c_bool,
            OnInitializePlatform,
            OnFinalizePlatform,
            OnHasMapping,
            OnNativeProxyCall,
            OnNativeProxyCallback,
        ]
        log.debug("OK")

        log.debug("Binding: xplpc_core_finalize...")
        PlatformProxy.xplpc_core_finalize = libc.xplpc_core_finalize
        log.debug("OK")

        log.debug("Binding: xplpc_core_is_initialized...")
        PlatformProxy.xplpc_core_is_initialized = libc.xplpc_core_is_initialized
        log.debug("OK")

        log.debug("Binding: xplpc_native_call_proxy...")
        PlatformProxy.xplpc_native_call_proxy = libc.xplpc_native_call_proxy
        PlatformProxy.xplpc_native_call_proxy.argtypes = [
            c_char_p,
            c_size_t,
            c_char_p,
            c_size_t,
        ]
        log.debug("OK")

        log.debug("Binding: xplpc_native_call_proxy_callback...")
        PlatformProxy.xplpc_native_call_proxy_callback = (
            libc.xplpc_native_call_proxy_callback
        )
        PlatformProxy.xplpc_native_call_proxy_callback.argtypes = [
            c_char_p,
            c_size_t,
            c_char_p,
            c_size_t,
        ]
        log.debug("OK")

        log.debug("Binding with Python...")
        initialize_callback = OnInitializePlatform(PlatformProxy.onInitializePlatform)
        finalize_callback = OnFinalizePlatform(PlatformProxy.onFinalizePlatform)
        has_mapping_callback = OnHasMapping(PlatformProxy.onHasMapping)
        native_proxy_call_callback = OnNativeProxyCall(PlatformProxy.onNativeProxyCall)
        native_proxy_callback_callback = OnNativeProxyCallback(
            PlatformProxy.onNativeProxyCallback
        )
        log.debug("OK")

        log.debug("Calling xplpc_core_initialize...")
        PlatformProxy.xplpc_core_initialize(
            True,
            initialize_callback,
            finalize_callback,
            has_mapping_callback,
            native_proxy_call_callback,
            native_proxy_callback_callback,
        )
        log.debug("OK")

        log.debug("Calling xplpc_core_is_initialized...")
        is_init = PlatformProxy.xplpc_core_is_initialized()
        log.debug(f"Is initialized? {is_init}")
        log.debug("OK")

        log.debug("Calling xplpc_native_call_proxy...")
        data = {"f": "sample.login"}
        data = json.dumps(data).encode("utf-8")
        PlatformProxy.xplpc_native_call_proxy(b"key", len(b"key"), data, len(data))
        log.debug("OK")

        log.debug("Calling xplpc_native_call_proxy...")
        PlatformProxy.xplpc_native_call_proxy_callback(
            b"key", len(b"key"), b"data", len(b"data")
        )
        log.debug("OK")

        log.debug("Calling _call_native_proxy_with...")
        PlatformProxy._call_native_proxy_with("key", "data")
        log.debug("OK")

        log.debug("Calling xplpc_core_finalize...")
        PlatformProxy.xplpc_core_finalize()
        log.debug("OK")

    @staticmethod
    def onInitializePlatform():
        print("Platform initialized")
        pass

    @staticmethod
    def onFinalizePlatform():
        print("Platform finalized")
        MappingList().clear()

    @staticmethod
    def onHasMapping(name, nameSize):
        print(f"Checking mapping: {name}, {nameSize}")
        return MappingList().has(name)

    @staticmethod
    def onNativeProxyCall(key, keySize, data, dataSize):
        from xplpc.core.xplpc import XPLPC

        print(f"Native proxy call: {key}, {keySize}, {data}, {dataSize}")

        keyStr = key[:keySize].decode()
        dataStr = data[:dataSize].decode()

        # function name
        functionName = XPLPC().config.serializer.decode_function_name(dataStr)

        if not functionName:
            log.error("[PlatformProxy : call] Function name is empty")
            PlatformProxy._call_native_proxy_with(keyStr, "")
            return

        # mapping item
        mappingItem = MappingList.instance.find(functionName)

        if not mappingItem:
            log.error(
                "[PlatformProxy : call] Mapping not found for function: " + functionName
            )
            PlatformProxy._call_native_proxy_with(keyStr, "")
            return

        # execute
        try:
            message = XPLPC.instance.config.serializer.decodeMessage(dataStr)
        except Exception as e:
            log.error(f"[PlatformProxy : call] Error when decode message: {e}")
            message = None

        if not message:
            log.error(
                f"[PlatformProxy : call] Error when decode message for function: {functionName}"
            )
            PlatformProxy._call_native_proxy_with(keyStr, "")
            return

        try:

            def callback(result):
                decodedData = XPLPC().config.serializer.encode_function_return_value(
                    result
                )

                PlatformProxy._call_native_proxy_with(keyStr, decodedData)

            mappingItem.target(message, callback)
        except Exception as e:
            log.error(f"[PlatformProxy : call] Error: {e}")
            PlatformProxy._call_native_proxy_with(keyStr, "")

    @staticmethod
    def onNativeProxyCallback(key, keySize, data, dataSize):
        keyStr = key[:keySize].decode()
        dataStr = data[:dataSize].decode()

        CallbackList().execute(keyStr, dataStr)

    @staticmethod
    def _call_native_proxy_with(key, data):
        # convert to UTF-8 bytes
        key_bytes = key.encode("utf-8")
        data_bytes = data.encode("utf-8")

        # call native function
        PlatformProxy.xplpc_native_call_proxy_callback(
            key_bytes, len(key_bytes), data_bytes, len(data_bytes)
        )
