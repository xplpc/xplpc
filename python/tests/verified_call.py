import threading

from xplpc.client.client import Client
from xplpc.message.request import Request


def verify_call(request: Request, class_type=None, timeout: float = 5.0):
    # A callback that never runs would leave every check unexecuted, so the answer is waited for and its arrival is asserted.

    answered = threading.Event()
    box = {}

    def callback(response):
        box["value"] = response
        answered.set()

    Client.call(request, callback, class_type)

    assert answered.wait(timeout), "the call was never answered"

    return box["value"]


def verify_call_from_string(request_data: str, timeout: float = 5.0):
    answered = threading.Event()
    box = {}

    def callback(response):
        box["value"] = response
        answered.set()

    Client.call_from_string(request_data, callback)

    assert answered.wait(timeout), "the call was never answered"

    return box["value"]
