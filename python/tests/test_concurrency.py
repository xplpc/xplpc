import logging
import threading

import pytest
from verified_call import verify_call
from xplpc.client.client import Client
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.data.mapping_list import MappingList
from xplpc.helper.byte_array_helper import ByteArrayHelper
from xplpc.layer.layer import Layer
from xplpc.map.mapping_item import MappingItem
from xplpc.message.message import Message
from xplpc.message.param import Param
from xplpc.message.request import Request
from xplpc.message.response import Response
from xplpc.serializer.json_serializer import JsonSerializer
from xplpc.type.dataview import DataView


@pytest.fixture(scope="session", autouse=True)
def setup_and_teardown_session():
    logging.basicConfig(level=logging.DEBUG)

    serializer = JsonSerializer()
    config = Config(serializer)
    XPLPC().initialize(config)

    yield


@pytest.fixture(autouse=True)
def setup_and_teardown():
    yield

    # A call left waiting is the defect this project has found most often, so every test is held to leaving none.
    assert Layer().pending_count() == 0

    # A view never released keeps its source for the life of the process, so every test is held to releasing what it registered.
    assert Layer().buffer_count() == 0


def battery_level(m: Message, r: Response):
    suffix = m.get("suffix")
    r(f"100{suffix}")


def reverse(m: Message, r: Response):
    r("ok")


def battery_level_worker():
    MappingList().add(
        "platform.battery.level",
        MappingItem(battery_level),
    )

    request = Request(
        "platform.battery.level",
        [
            Param("suffix", "%"),
        ],
    )

    response = verify_call(request)
    assert response == "100%"


def reverse_worker():
    MappingList().add(
        "platform.reverse.response",
        MappingItem(reverse),
    )

    request = Request("sample.reverse")

    response = verify_call(request)
    assert response == "response-is-ok"


def grayscale_image_with_dataView_worker():
    data = bytearray(
        [
            255,
            0,
            0,
            255,
            0,
            255,
            0,
            255,
            0,
            0,
            255,
            255,
            0,
            0,
            0,
            0,
        ]
    )

    data_view = DataView.create_from_byte_buffer(data)

    request = Request(
        "sample.image.grayscale.dataview",
        [
            Param("dataView", data_view),
        ],
    )

    response = verify_call(request)
    assert response == "OK"

    read = ByteArrayHelper.create_from_data_view(data_view)

    assert data_view.release()

    for grey in (data, read):
        assert len(grey) == 16
        assert grey[0] == 85
        assert grey[4] == 85
        assert grey[8] == 85
        assert grey[12] == 0


@pytest.mark.asyncio
async def test_battery_level_concurrent():
    threads = []

    for _ in range(100):
        thread = threading.Thread(target=battery_level_worker)
        threads.append(thread)
        thread.start()

    for thread in threads:
        thread.join()


@pytest.mark.asyncio
async def test_reverse_concurrent():
    threads = []

    for _ in range(100):
        thread = threading.Thread(target=reverse_worker)
        threads.append(thread)
        thread.start()

    for thread in threads:
        thread.join()


@pytest.mark.asyncio
async def test_grayscale_image_with_dataView_worker_concurrent():
    threads = []

    for _ in range(100):
        thread = threading.Thread(target=grayscale_image_with_dataView_worker)
        threads.append(thread)
        thread.start()

    for thread in threads:
        thread.join()


# The mapping answers after the call returned, from a thread it created, and still has to reach python.
@pytest.mark.asyncio
async def test_core_mapping_answers_from_its_own_thread():
    answered = threading.Event()

    Client.call(Request("sample.async"), lambda *_: answered.set())

    assert answered.wait(5)


def test_initializing_from_many_threads_keeps_one_set_of_callbacks():
    # The native side is handed the ctypes callback objects the layer holds, so replacing them while it still points at the old ones is a dangling function pointer.

    layer = Layer()

    before = (layer.invoke_callback, layer.post_callback, layer.host, layer.handle)

    threads = [threading.Thread(target=layer.initialize) for _ in range(16)]

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

    after = (layer.invoke_callback, layer.post_callback, layer.host, layer.handle)

    for original, current in zip(before, after, strict=True):
        assert original is current


def test_initializing_the_library_from_many_threads_keeps_the_first_config():
    serializer = XPLPC().config.serializer

    threads = [
        threading.Thread(target=XPLPC().initialize, args=(Config(JsonSerializer()),))
        for _ in range(16)
    ]

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

    assert XPLPC().config.serializer is serializer
    assert XPLPC().initialized


@pytest.mark.parametrize("singleton", [MappingList, Layer, XPLPC])
def test_a_singleton_is_never_handed_out_before_it_is_built(singleton, monkeypatch):
    # A second caller that does not wait for the first one receives an object whose fields are not there yet.

    build = singleton._init
    building = threading.Event()
    release = threading.Event()
    observed = {}

    def slow_build(self):
        building.set()
        release.wait(5)
        build(self)

    def observe():
        observed["complete"] = hasattr(singleton(), "lock")

    monkeypatch.setattr(singleton, "_instance", None)
    monkeypatch.setattr(singleton, "_init", slow_build)

    first = threading.Thread(target=singleton)
    first.start()

    assert building.wait(5)

    second = threading.Thread(target=observe)
    second.start()
    second.join(0.2)

    release.set()

    first.join()
    second.join()

    assert observed["complete"]
