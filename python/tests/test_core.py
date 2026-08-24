import logging

import pytest
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.data.callback_list import CallbackList
from xplpc.data.mapping_list import MappingList
from xplpc.map.mapping_item import MappingItem
from xplpc.serializer.json_serializer import JsonSerializer


@pytest.fixture(scope="session", autouse=True)
def setup_and_teardown_session():
    logging.basicConfig(level=logging.DEBUG)

    yield


@pytest.fixture(autouse=True)
def setup_and_teardown():
    yield

    # A leaked callback is the defect this project has found most often, so every test is held to leaving none.
    assert CallbackList().count() == 0


def test_core_initialize():
    serializer = JsonSerializer()
    config = Config(serializer)
    XPLPC().initialize(config)

    assert XPLPC().is_initialized()


def test_a_mapping_that_cannot_be_declared_leaves_nothing_behind():
    # The native side routes from the names it was told, so a name recorded locally but never
    # declared would be unreachable for the life of the process with nothing reporting it.

    XPLPC().initialize(Config(JsonSerializer()))

    name = "platform.\ud800"
    before = MappingList().count()

    with pytest.raises(UnicodeEncodeError):
        MappingList().add(name, MappingItem(lambda message, r: r(None)))

    assert not MappingList().has(name)
    assert MappingList().count() == before
