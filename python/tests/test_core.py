import logging

import pytest

from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.serializer.json_serializer import JsonSerializer

# ------------------------------------------------------------------------------
# FIXTURES
# ------------------------------------------------------------------------------


# general fixture before and after all tests
@pytest.fixture(scope="session", autouse=True)
def setup_and_teardown_session():
    # general command before all tests
    logging.basicConfig(level=logging.DEBUG)

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


def test_core_initialize():
    serializer = JsonSerializer()
    config = Config(serializer)
    XPLPC().initialize(config)

    assert XPLPC().is_initialized()
