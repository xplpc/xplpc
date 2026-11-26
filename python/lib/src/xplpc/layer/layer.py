import atexit
import os
import platform
import sys
import threading
from ctypes import (
    CFUNCTYPE,
    POINTER,
    Structure,
    addressof,
    byref,
    c_bool,
    c_char,
    c_int,
    c_size_t,
    c_uint64,
    c_void_p,
    cdll,
    create_string_buffer,
    string_at,
)

from xplpc.data.mapping_list import MappingList
from xplpc.util.log import Log
from xplpc.util.singleton import Singleton

LIBRARY_PATH_VARIABLE = "XPLPC_LIBRARY_PATH"

POST_ANSWER = 0

BUFFER_TYPE = POINTER(c_char)
INVOKE_TYPE = CFUNCTYPE(None, c_void_p, c_uint64, BUFFER_TYPE, c_size_t)
POST_TYPE = CFUNCTYPE(None, c_void_p, c_int, c_uint64, BUFFER_TYPE, c_size_t)


class _Host(Structure):
    _fields_ = [
        ("context", c_void_p),
        ("invoke", INVOKE_TYPE),
        ("post", POST_TYPE),
        ("enterableAnywhere", c_bool),
        ("name", BUFFER_TYPE),
        ("nameSize", c_size_t),
    ]


class _CallResult(Structure):
    _fields_ = [
        ("call", c_uint64),
        ("data", BUFFER_TYPE),
        ("dataSize", c_size_t),
    ]


class PendingCall:
    def __init__(self, callback):
        self.callback = callback
        self.call = 0


class Layer(Singleton):
    def _init(self):
        self.lock = threading.Lock()
        self.libc = None
        self.handle = 0
        self.pending = {}
        self.buffers = {}

        self.invoke_callback = None
        self.post_callback = None
        self.host = None

    def initialize(self):
        with self.lock:
            if self.libc:
                return

            self._bind()

    def _bind(self):
        # Every symbol is resolved before anything is published, so a library that lacks one leaves this layer unbound and a later attempt binds it again.

        libc = cdll.LoadLibrary(self.get_lib_path())

        libc.xplpc_attach.argtypes = [POINTER(_Host)]
        libc.xplpc_attach.restype = c_uint64
        libc.xplpc_detach.argtypes = [c_uint64]
        libc.xplpc_declare.argtypes = [c_uint64, BUFFER_TYPE, c_size_t]
        libc.xplpc_retract.argtypes = [c_uint64, BUFFER_TYPE, c_size_t]
        libc.xplpc_call.argtypes = [
            c_uint64,
            c_uint64,
            BUFFER_TYPE,
            c_size_t,
            POINTER(_CallResult),
        ]
        libc.xplpc_call.restype = c_bool
        libc.xplpc_cancel.argtypes = [c_uint64, c_uint64]
        libc.xplpc_cancel.restype = c_bool
        libc.xplpc_answer.argtypes = [c_uint64, BUFFER_TYPE, c_size_t]
        libc.xplpc_buffer_register.argtypes = [c_uint64, c_void_p, c_size_t]
        libc.xplpc_buffer_register.restype = c_uint64
        libc.xplpc_buffer_read.argtypes = [
            c_uint64,
            POINTER(c_void_p),
            POINTER(c_size_t),
        ]
        libc.xplpc_buffer_read.restype = c_bool
        libc.xplpc_buffer_release.argtypes = [c_uint64, c_uint64]
        libc.xplpc_buffer_release.restype = c_bool
        libc.xplpc_free.argtypes = [c_void_p]
        libc.xplpc_pending.argtypes = []
        libc.xplpc_pending.restype = c_size_t

        # The native side keeps pointers into these objects from the moment it receives them, so they are held here for as long as the library is loaded.
        self.invoke_callback = INVOKE_TYPE(self._on_invoke)
        self.post_callback = POST_TYPE(self._on_post)
        self.host = _Host(None, self.invoke_callback, self.post_callback, True, None, 0)

        handle = libc.xplpc_attach(byref(self.host))

        if not handle:
            raise RuntimeError("XPLPC could not attach the python layer")

        self.handle = handle
        self.libc = libc

        # A native thread can answer after the interpreter finalized, so the layer leaves the router while there is still an interpreter to receive what that sends.
        atexit.register(self._detach)

    def _detach(self):
        self.libc.xplpc_detach(self.handle)

    def _on_invoke(self, _context, call, data, data_size):
        self._serve(call, string_at(data, data_size))

    def _on_post(self, _context, kind, identifier, data, data_size):
        # The bytes belong to this side once they arrive, so they are copied and released before anything that could raise.
        # This layer attaches as one that can be entered from any thread, so a call always reaches it through invoke and only a late answer or the release arrives here.

        payload = string_at(data, data_size) if data else b""

        if data:
            self.libc.xplpc_free(data)

        if kind == POST_ANSWER:
            self._deliver(identifier, payload)

    def _serve(self, call, payload):
        from xplpc.core.xplpc import XPLPC

        # This runs inside a ctypes callback, where an exception reaches nobody and the caller would wait forever, so every failure answers instead.

        respond = self._responder(call)

        try:
            data = payload.decode()
        except UnicodeDecodeError as e:
            Log.e("[Layer : invoke] Unable to decode data")
            Log.d(f"[Layer : invoke] Unable to decode data: {e}")
            respond("")
            return

        if not XPLPC().initialized:
            Log.e("[Layer : invoke] XPLPC was not initialized")
            respond("")
            return

        serializer = XPLPC().config.serializer

        try:
            request = serializer.decode_request(data)
            mapping_item = (
                MappingList().find(request.function_name) if request else None
            )
        except Exception as e:
            Log.e("[Layer : invoke] Error when decode request")
            Log.d(f"[Layer : invoke] Error when decode request: {e}")
            respond("")
            return

        if not request:
            respond("")
            return

        if not mapping_item:
            Log.e(
                f"[Layer : invoke] Mapping not found for function: {request.function_name}"
            )
            respond("")
            return

        function_name = request.function_name

        def answer(result):
            try:
                encoded = serializer.encode_function_return_value(result)
            except Exception as e:
                Log.e(
                    f'[Layer : invoke] Error when encode the answer of function "{function_name}"'
                )
                Log.d(
                    f'[Layer : invoke] Error when encode the answer of function "{function_name}": {e}'
                )
                encoded = ""

            respond(encoded)

        # A ctypes callback swallows whatever leaves it, so even SystemExit and KeyboardInterrupt are answered here rather than leaving the caller waiting.
        try:
            mapping_item.target(request.message, answer)
        except BaseException as e:
            Log.e(f'[Layer : invoke] Error when execute function "{function_name}"')
            Log.d(
                f'[Layer : invoke] Error when execute function "{function_name}": {e}'
            )
            respond("")

    def _responder(self, call):
        # The router takes the call on its first answer, so a mapping that answers and then raises, or answers twice, reaches the caller a single time.

        def respond(data):
            try:
                encoded = data.encode("utf-8")
            except UnicodeEncodeError as e:
                # A mapping may answer with text read off the filesystem, which carries a surrogate utf8 cannot represent.
                Log.e("[Layer : invoke] Error when encode the answer")
                Log.d(f"[Layer : invoke] Error when encode the answer: {e}")
                encoded = b""

            self.libc.xplpc_answer(call, create_string_buffer(encoded), len(encoded))

        return respond

    def _deliver(self, token, payload):
        with self.lock:
            pending = self.pending.pop(token, None)

        if not pending:
            return

        # A late answer is delivered inside a ctypes callback, where anything the callback raises reaches nobody, SystemExit and KeyboardInterrupt included.
        self._run(pending, self._text(payload), BaseException)

    def _text(self, payload):
        try:
            return payload.decode()
        except UnicodeDecodeError as e:
            Log.e("[Layer : answer] Unable to decode data")
            Log.d(f"[Layer : answer] Unable to decode data: {e}")

        return ""

    def _run(self, pending, response, reported=Exception):
        try:
            pending.callback(response)
        except reported as e:
            Log.e("[Layer : answer] The callback raised while being answered")
            Log.d(f"[Layer : answer] The callback raised while being answered: {e}")

    def _require_bound(self, source):
        if self.libc:
            return

        Log.e(f"[Layer : {source}] XPLPC was not initialized")

        raise RuntimeError("XPLPC was not initialized")

    def declare(self, name):
        self._require_bound("declare")

        name_bytes = name.encode("utf-8")
        self.libc.xplpc_declare(
            self.handle, create_string_buffer(name_bytes), len(name_bytes)
        )

    def retract(self, name):
        self._require_bound("retract")

        name_bytes = name.encode("utf-8")
        self.libc.xplpc_retract(
            self.handle, create_string_buffer(name_bytes), len(name_bytes)
        )

    def call(self, data, pending):
        # The entry is registered before the call is made, since a late answer can be posted from another thread before the call returns, and it is keyed by the identity of the object it holds.

        self._require_bound("call")

        data_bytes = data.encode("utf-8")
        token = id(pending)

        with self.lock:
            self.pending[token] = pending

        result = _CallResult()

        try:
            answered = self.libc.xplpc_call(
                self.handle,
                token,
                create_string_buffer(data_bytes),
                len(data_bytes),
                byref(result),
            )
        except BaseException:
            with self.lock:
                self.pending.pop(token, None)

            raise

        if not answered:
            pending.call = result.call
            return

        with self.lock:
            self.pending.pop(token, None)

        payload = string_at(result.data, result.dataSize) if result.data else b""

        if result.data:
            self.libc.xplpc_free(result.data)

        self._run(pending, self._text(payload))

    def cancel(self, pending):
        # Taking the call back is what decides between the answer and the cancel, since whichever loses resolves nothing.

        if not pending.call or not self.libc.xplpc_cancel(self.handle, pending.call):
            return False

        with self.lock:
            self.pending.pop(id(pending), None)

        return True

    def register_buffer(self, source):
        # The native side reads the bytes in place until the handle is released, so the exported source is held here until then, and a buffer never released is kept rather than freed under a reader.

        self._require_bound("register_buffer")

        with memoryview(source) as exported:
            size = exported.nbytes

        storage = (c_char * size).from_buffer(source)

        with self.lock:
            handle = self.libc.xplpc_buffer_register(
                self.handle, addressof(storage), size
            )

            if handle:
                self.buffers[handle] = storage

        return handle

    def release_buffer(self, handle):
        # The native side lets go of the handle before the source is, so no reader can reach bytes this side no longer holds.

        self._require_bound("release_buffer")

        # A view that names nothing already reported why where it was made, so there is nothing to release and nothing to report again.
        if not handle:
            return False

        with self.lock:
            if not self.libc.xplpc_buffer_release(self.handle, handle):
                return False

            storage = self.buffers.pop(handle, None)

        # Letting go of the source can run code of the caller's own, so it happens outside the lock.
        del storage

        return True

    def read_buffer(self, handle):
        # A handle naming nothing, released or never registered reads as no bytes, and what is read is a copy, since the owner may release the bytes right afterwards.

        self._require_bound("read_buffer")

        if not handle:
            return b""

        data = c_void_p()
        size = c_size_t()

        if not self.libc.xplpc_buffer_read(handle, byref(data), byref(size)):
            return b""

        return string_at(data.value, size.value)

    def buffer_count(self):
        # The buffers this side registered and has not released yet, which is what lets a test prove it left none behind.

        with self.lock:
            return len(self.buffers)

    def pending_count(self):
        # The router counts the calls waiting anywhere in the process, which includes the ones the c++ side is holding, and this side adds the callers it still has to answer.

        self._require_bound("pending_count")

        with self.lock:
            waiting = len(self.pending)

        return self.libc.xplpc_pending() + waiting

    def get_lib_path(self):
        # The library ships inside the package, and a consumer that keeps it somewhere the package does not names that place in the environment.

        configured = os.environ.get(LIBRARY_PATH_VARIABLE)

        if configured:
            return configured

        return os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
            "lib",
            self._lib_arch(),
            self._lib_name(),
        )

    def _lib_name(self):
        plat = sys.platform.lower()

        if plat.startswith("win"):
            return "xplpc.dll"

        if plat.startswith("darwin"):
            return "libxplpc.dylib"

        if plat.startswith("linux"):
            return "libxplpc.so"

        raise OSError(f"The platform {plat} is not supported.")

    def _lib_arch(self):
        arch = platform.machine().lower()

        if arch in ("armv7l", "armv7"):
            return "arm32"

        if arch in ("aarch64", "arm64"):
            return "arm64"

        if arch in ("i686", "x86"):
            return "x86"

        if arch in ("x86_64", "amd64"):
            return "x86_64"

        raise OSError(f"The architecture {arch} is not supported.")
