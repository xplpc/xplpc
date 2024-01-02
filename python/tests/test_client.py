import asyncio
import logging

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


def raise_assertion_error(message):
    pytest.fail(message)


# ------------------------------------------------------------------------------
# TESTS
# ------------------------------------------------------------------------------


def test_battery_level():
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


@pytest.mark.asyncio
async def test_battery_level_async():
    MappingList().add(
        "platform.battery.level",
        MappingItem(battery_level_async),
    )

    request = Request(
        "platform.battery.level",
        [
            Param("suffix", "%"),
        ],
    )

    response = await Client.call_async(request)
    assert response == "100%"


def test_login():
    request = Request(
        "sample.login",
        [
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", True),
        ],
    )

    response = Client.call(request)
    assert response == "LOGGED-WITH-REMEMBER"


@pytest.mark.asyncio
async def test_login_async():
    request = Request(
        "sample.login",
        [
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", True),
        ],
    )

    response = await Client.call_async(request)
    assert response == "LOGGED-WITH-REMEMBER"


def test_login_type():
    request = Request(
        "sample.login",
        [
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", True),
        ],
    )

    response = Client.call(request)
    assert str == type(response)


def test_reverse():
    MappingList().add(
        "platform.reverse.response",
        MappingItem(reverse),
    )

    request = Request("sample.reverse")

    response = Client.call(request)
    assert response == "response-is-ok"


@pytest.mark.asyncio
async def test_reverse_async():
    MappingList().add(
        "platform.reverse.response",
        MappingItem(reverse),
    )

    request = Request("sample.reverse")

    response = await Client.call_async(request)
    assert response == "response-is-ok"


def test_grayscale_image_with_dataView():
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


@pytest.mark.asyncio
async def test_grayscale_image_with_dataView_async():
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

    response = await Client.call_async(request)
    assert response == "OK"

    data = ByteArrayHelper.create_from_data_view(data_view)

    assert 16 == data_view.size
    assert data[0] == 85
    assert data[4] == 85
    assert data[8] == 85
    assert data[12] == 0


def test_data_view():
    # get data view
    request = Request("sample.dataview")
    data_view = Client.call(request, DataView)

    # check that data view is not None
    assert data_view is not None

    # check current values
    original_data = ByteArrayHelper.create_from_data_view(data_view)

    assert len(original_data) == 16
    assert original_data[0] == 255
    assert original_data[3] == 255
    assert original_data[7] == 255
    assert original_data[12] == 0

    # send original data and check modified data
    data_view2 = DataView.create_from_byte_buffer(original_data, len(original_data))

    request2 = Request(
        "sample.image.grayscale.dataview",
        [
            Param("dataView", data_view2),
        ],
    )

    response2 = Client.call(request2)
    assert response2 == "OK"

    processed_data = ByteArrayHelper.create_from_data_view(data_view2)

    # check copied values
    assert data_view2.size == 16
    assert processed_data[0] == 85
    assert processed_data[4] == 85
    assert processed_data[8] == 85
    assert processed_data[12] == 0

    # check original values again
    assert len(original_data) == 16
    assert original_data[0] == 85
    assert original_data[4] == 85
    assert original_data[8] == 85
    assert original_data[12] == 0


@pytest.mark.asyncio
async def test_data_view_async():
    # get data view
    request = Request("sample.dataview")
    data_view = await Client.call_async(request, DataView)

    # check that data view is not None
    assert data_view is not None

    # check current values
    original_data = ByteArrayHelper.create_from_data_view(data_view)

    assert len(original_data) == 16
    assert original_data[0] == 255
    assert original_data[3] == 255
    assert original_data[7] == 255
    assert original_data[12] == 0

    # send original data and check modified data
    data_view2 = DataView.create_from_byte_buffer(original_data, len(original_data))

    request2 = Request(
        "sample.image.grayscale.dataview",
        [
            Param("dataView", data_view2),
        ],
    )

    response2 = await Client.call_async(request2)
    assert response2 == "OK"

    processed_data = ByteArrayHelper.create_from_data_view(data_view2)

    # check copied values
    assert data_view2.size == 16
    assert processed_data[0] == 85
    assert processed_data[4] == 85
    assert processed_data[8] == 85
    assert processed_data[12] == 0

    # check original values again
    assert len(original_data) == 16
    assert original_data[0] == 85
    assert original_data[4] == 85
    assert original_data[8] == 85
    assert original_data[12] == 0


def test_battery_level_from_string():
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

    response = Client.call_from_string(request.data())

    assert '{"r": "100%"}' == response


@pytest.mark.asyncio
async def test_battery_level_from_string_async():
    MappingList().add(
        "platform.battery.level",
        MappingItem(battery_level_async),
    )

    request = Request(
        "platform.battery.level",
        [
            Param("suffix", "%"),
        ],
    )

    response = await Client.call_async_from_string(request.data())

    assert '{"r": "100%"}' == response
