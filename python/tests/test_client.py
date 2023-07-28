import logging

import pytest

from xplpc.client.client import Client
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.data.mapping_list import MappingList
from xplpc.map.mapping_item import MappingItem
from xplpc.message.message import Message
from xplpc.message.param import Param
from xplpc.message.request import Request
from xplpc.message.response import Response
from xplpc.serializer.json_serializer import JsonSerializer

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

    Client.call(
        request,
        lambda response: response == "100%"
        or raise_assertion_error(f"'{response}' not equal to '100%'"),
    )


def test_reverse():
    MappingList().add(
        "platform.reverse.response",
        MappingItem(reverse),
    )

    request = Request("sample.reverse")

    Client.call(
        request,
        lambda response: response == "response-is-ok"
        or raise_assertion_error(f"'{response}' not equal to 'response-is-ok'"),
    )
