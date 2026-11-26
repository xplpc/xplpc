from ctypes import create_string_buffer, string_at

from xplpc.layer.layer import Layer


def invoke_layer(monkeypatch, data: bytes, call: int = 7):
    # The router is left out, so what the layer answers is captured where it would otherwise reach the native side.

    answers = []

    def capture(handle, response, size):
        answers.append((handle, string_at(response, size).decode()))

    monkeypatch.setattr(Layer().libc, "xplpc_answer", capture)

    buffer = create_string_buffer(data, len(data))
    Layer().invoke_callback(None, call, buffer, len(data))

    return answers


def take_the_only_pending():
    # This is what the layer does before delivering an answer, which is the window an answer races a cancel or a closing loop in.

    with Layer().lock:
        _, pending = Layer().pending.popitem()

    return pending
