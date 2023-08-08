import asyncio
import logging
import threading

import pytest

from xplpc.client.client import Client
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.data.mapping_list import MappingList
from xplpc.helper.byte_array_helper import ByteArrayHelper
from xplpc.map.mapping_item import MappingItem
from xplpc.message.message import Message
from xplpc.message.param import Param
from xplpc.message.request import Request
from xplpc.message.response import Response
from xplpc.serializer.json_serializer import JsonSerializer
from xplpc.type.dataview import DataView

# ------------------------------------------------------------------------------
# FIXTURES
# ------------------------------------------------------------------------------


# general fixture before and after all tests
@pytest.fixture(scope="session", autouse=True)
def setup_and_teardown_session():
    # general command before all tests
    logging.basicConfig(level=logging.DEBUG)

    serializer = JsonSerializer()
    config = Config(serializer)
    XPLPC().initialize(config)

    yield

    # general command after all tests


# fixture before and after each individual test
@pytest.fixture(autouse=True)
def setup_and_teardown():
    # command before each test
    yield
    # command after each test


# ------------------------------------------------------------------------------
# CALLBACKS
# ------------------------------------------------------------------------------


def battery_level(m: Message, r: Response):
    suffix = m.get("suffix")
    r(f"100{suffix}")


def battery_level_async(m: Message, r: Response):
    async def main():
        # async sleep
        await asyncio.sleep(0.1)

        # return response
        suffix = m.get("suffix")
        r(f"100{suffix}")

    # create a new event loop
    loop = asyncio.new_event_loop()

    try:
        loop.run_until_complete(main())
    finally:
        loop.close()


def reverse(m: Message, r: Response):
    r("ok")


# ------------------------------------------------------------------------------
# WORKERS
# ------------------------------------------------------------------------------


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

    response = Client.call(request)
    assert response == "100%"


def reverse_worker():
    MappingList().add(
        "platform.reverse.response",
        MappingItem(reverse),
    )

    request = Request("sample.reverse")

    response = Client.call(request)
    assert response == "response-is-ok"


def grayscale_image_with_dataView_worker():
    data = bytearray(
        [
            # red pixel
            255,
            0,
            0,
            255,
            # green pixel
            0,
            255,
            0,
            255,
            # blue pixel
            0,
            0,
            255,
            255,
            # transparent pixel
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

    response = Client.call(request)
    assert response == "OK"

    data = ByteArrayHelper.create_from_data_view(data_view)

    assert 16 == data_view.size
    assert data[0] == 85
    assert data[4] == 85
    assert data[8] == 85
    assert data[12] == 0


# ------------------------------------------------------------------------------
# TESTS
# ------------------------------------------------------------------------------


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
