import os
import platform
import sys
import threading
from ctypes import (
    CFUNCTYPE,
    POINTER,
    c_bool,
    c_char,
    c_size_t,
    c_void_p,
    cdll,
    create_string_buffer,
)

import xplpc.util.platform as p
from xplpc.data.callback_list import CallbackList
from xplpc.data.mapping_list import MappingList
from xplpc.util.log import Log


class PlatformProxy:
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
        self.libc = None
        self.xplpc_core_initialize = None
        self.xplpc_native_call_proxy = None
        self.xplpc_native_call_proxy_callback = None
        self.xplpc_native_add_mapping = None
        self.xplpc_native_clear_mappings = None
        self.xplpc_free = None

        self.initialize_callback = None
        self.finalize_callback = None
        self.native_proxy_call_callback = None
        self.native_proxy_callback_callback = None
        self.native_proxy_call_from_thread_callback = None
        self.native_proxy_callback_from_thread_callback = None

    def initialize(self):
        with self.lock:
            if self.libc:
                return

            self._bind()

    def _bind(self):
        # The native side is handed the ctypes callback objects this holds, so replacing them while it still points at the old ones is a dangling function pointer.

        lib_path = self.get_lib_path()
        self.libc = cdll.LoadLibrary(lib_path)

        on_initialize_platform_type = CFUNCTYPE(None)
        on_finalize_platform_type = CFUNCTYPE(None)
        buffer_type = POINTER(c_char)

        on_native_proxy_callback = CFUNCTYPE(
            None, buffer_type, c_size_t, buffer_type, c_size_t
        )
        on_native_proxy_call = CFUNCTYPE(
            None, buffer_type, c_size_t, buffer_type, c_size_t
        )

        self.xplpc_core_initialize = self.libc.xplpc_core_initialize
        self.xplpc_core_initialize.argtypes = [
            c_bool,
            on_initialize_platform_type,
            on_finalize_platform_type,
            on_native_proxy_call,
            on_native_proxy_callback,
            on_native_proxy_call,
            on_native_proxy_callback,
        ]

        self.xplpc_native_call_proxy = self.libc.xplpc_native_call_proxy
        self.xplpc_native_call_proxy.argtypes = [
            buffer_type,
            c_size_t,
            buffer_type,
            c_size_t,
        ]

        self.xplpc_native_call_proxy_callback = (
            self.libc.xplpc_native_call_proxy_callback
        )
        self.xplpc_native_call_proxy_callback.argtypes = [
            buffer_type,
            c_size_t,
            buffer_type,
            c_size_t,
        ]

        self.xplpc_native_add_mapping = self.libc.xplpc_native_add_mapping
        self.xplpc_native_add_mapping.argtypes = [buffer_type, c_size_t]

        self.xplpc_native_clear_mappings = self.libc.xplpc_native_clear_mappings

        self.xplpc_free = self.libc.xplpc_free
        self.xplpc_free.argtypes = [c_void_p]

        self.initialize_callback = on_initialize_platform_type(
            self.on_initialize_platform
        )
        self.finalize_callback = on_finalize_platform_type(self.on_finalize_platform)
        self.native_proxy_call_callback = on_native_proxy_call(
            self.on_native_proxy_call
        )
        self.native_proxy_callback_callback = on_native_proxy_callback(
            self.on_native_proxy_callback
        )
        self.native_proxy_call_from_thread_callback = on_native_proxy_call(
            self.on_native_proxy_call_from_thread
        )
        self.native_proxy_callback_from_thread_callback = on_native_proxy_callback(
            self.on_native_proxy_callback_from_thread
        )

        self.xplpc_core_initialize(
            True,
            self.initialize_callback,
            self.finalize_callback,
            self.native_proxy_call_callback,
            self.native_proxy_callback_callback,
            self.native_proxy_call_from_thread_callback,
            self.native_proxy_callback_from_thread_callback,
        )

    def on_initialize_platform(self):
        pass

    def on_finalize_platform(self):
        MappingList().clear()

    def on_native_proxy_call(self, key, key_size, data, data_size):
        from xplpc.core.xplpc import XPLPC

        try:
            key_str = key[:key_size].decode()
        except UnicodeDecodeError:
            # There is nothing to answer under a key that cannot be read, so the call is reported and dropped.
            Log.e("[PlatformProxy : call] Unable to decode key")
            Log.d(f"[PlatformProxy : call] Unable to decode key: {key[:key_size]}")
            return

        try:
            data_str = data[:data_size].decode()
        except UnicodeDecodeError:
            Log.e("[PlatformProxy : call] Unable to decode data")
            Log.d(f"[PlatformProxy : call] Unable to decode data: {data[:data_size]}")
            self.call_native_proxy_callback(key_str, "")
            return

        if not XPLPC().is_initialized():
            Log.e("[PlatformProxy : call] XPLPC was not initialized")
            self.call_native_proxy_callback(key_str, "")
            return

        request = XPLPC().config.serializer.decode_request(data_str)

        if not request:
            self.call_native_proxy_callback(key_str, "")
            return

        if not request.function_name:
            Log.e("[PlatformProxy : call] Function name is empty")
            self.call_native_proxy_callback(key_str, "")
            return

        mapping_item = MappingList().find(request.function_name)

        if not mapping_item:
            Log.e(
                "[PlatformProxy : call] Mapping not found for function: "
                + request.function_name
            )
            self.call_native_proxy_callback(key_str, "")
            return

        try:
            mapping_item.target(
                request.message,
                lambda result: self.call_native_proxy_callback(
                    key_str,
                    XPLPC().config.serializer.encode_function_return_value(result),
                ),
            )
        except Exception as e:
            Log.e(
                f'[PlatformProxy : call] Error when execute function "{request.function_name}"'
            )
            Log.d(
                f'[PlatformProxy : call] Error when execute function "{request.function_name}": {e}'
            )
            self.call_native_proxy_callback(key_str, "")

    def on_native_proxy_callback(self, key, key_size, data, data_size):
        try:
            key_decoded = key[:key_size].decode()
        except UnicodeDecodeError:
            Log.e("[PlatformProxy : on_native_proxy_callback] Unable to decode key")
            Log.d(
                f"[PlatformProxy : on_native_proxy_callback] Unable to decode key: {key[:key_size]}"
            )
            return

        try:
            data_decoded = data[:data_size].decode()
        except UnicodeDecodeError:
            Log.e("[PlatformProxy : on_native_proxy_callback] Unable to decode data")
            Log.d(
                f"[PlatformProxy : on_native_proxy_callback] Unable to decode data: {data[:data_size]}"
            )
            data_decoded = ""

        CallbackList().execute(key_decoded, data_decoded)

    def on_native_proxy_call_from_thread(self, key, key_size, data, data_size):
        # The buffers belong to this side once they arrive, so they are released after being read.

        try:
            self.on_native_proxy_call(key, key_size, data, data_size)
        finally:
            self.release_native_buffers(key, data)

    def on_native_proxy_callback_from_thread(self, key, key_size, data, data_size):
        try:
            self.on_native_proxy_callback(key, key_size, data, data_size)
        finally:
            self.release_native_buffers(key, data)

    def release_native_buffers(self, key, data):
        self.xplpc_free(key)
        self.xplpc_free(data)

    def _require_bound(self, source):
        if self.xplpc_native_add_mapping:
            return

        Log.e(f"[PlatformProxy : {source}] The library was not initialized")

        raise RuntimeError("XPLPC was not initialized")

    def add_mapping(self, name):
        # The native side answers has_mapping from these names, so it never has to reach python to resolve a function.

        self._require_bound("add_mapping")

        name_bytes = name.encode("utf-8")
        self.xplpc_native_add_mapping(create_string_buffer(name_bytes), len(name_bytes))

    def clear_mappings(self):
        self._require_bound("clear_mappings")

        self.xplpc_native_clear_mappings()

    def call_native_proxy_callback(self, key, data):
        key_bytes = key.encode("utf-8")
        data_bytes = data.encode("utf-8")

        self.xplpc_native_call_proxy_callback(
            create_string_buffer(key_bytes),
            len(key_bytes),
            create_string_buffer(data_bytes),
            len(data_bytes),
        )

    def call_native_proxy(self, key, data):
        key_bytes = key.encode("utf-8")
        data_bytes = data.encode("utf-8")

        self.xplpc_native_call_proxy(
            create_string_buffer(key_bytes),
            len(key_bytes),
            create_string_buffer(data_bytes),
            len(data_bytes),
        )

    def get_lib_path(self):
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
                lib_path = os.path.abspath(
                    os.path.join(
                        build_dir,
                        lib_arch,
                        lib_binary_dir,
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

        return lib_path
