import asyncio
import logging
import threading

import pytest
from verified_call import verify_call
from xplpc.client.client import Client
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.data.callback_list import CallbackList
from xplpc.data.mapping_list import MappingList
from xplpc.helper.byte_array_helper import ByteArrayHelper
from xplpc.map.mapping_item import MappingItem
from xplpc.message.message import Message
from xplpc.message.param import Param
from xplpc.message.request import Request
from xplpc.message.response import Response
from xplpc.proxy.platform_proxy import PlatformProxy
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

    # A leaked callback is the defect this project has found most often, so every test is held to leaving none.
    assert CallbackList().count() == 0


def battery_level(m: Message, r: Response):
    suffix = m.get("suffix")
    r(f"100{suffix}")


def battery_level_async(m: Message, r: Response):
    async def main():
        await asyncio.sleep(0.1)

        suffix = m.get("suffix")
        r(f"100{suffix}")

    loop = asyncio.new_event_loop()

    try:
        loop.run_until_complete(main())
    finally:
        loop.close()


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

    data_view = DataView.create_from_byte_buffer(data, len(data))

    request = Request(
        "sample.image.grayscale.dataview",
        [
            Param("dataView", data_view),
        ],
    )

    response = verify_call(request)
    assert response == "OK"

    data = ByteArrayHelper.create_from_data_view(data_view)

    assert 16 == data_view.size
    assert data[0] == 85
    assert data[4] == 85
    assert data[8] == 85
    assert data[12] == 0


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
    # The native side is handed the ctypes callback objects the proxy holds, so replacing them while it still points at the old ones is a dangling function pointer.

    proxy = PlatformProxy()

    before = (
        proxy.native_proxy_call_callback,
        proxy.native_proxy_callback_callback,
        proxy.native_proxy_call_from_thread_callback,
        proxy.native_proxy_callback_from_thread_callback,
    )

    threads = [threading.Thread(target=proxy.initialize) for _ in range(16)]

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

    after = (
        proxy.native_proxy_call_callback,
        proxy.native_proxy_callback_callback,
        proxy.native_proxy_call_from_thread_callback,
        proxy.native_proxy_callback_from_thread_callback,
    )

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
