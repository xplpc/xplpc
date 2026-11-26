import ctypes.util
import logging
import threading

import pytest
from xplpc.client.client import Client
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.data.mapping_list import MappingList
from xplpc.layer.layer import Layer
from xplpc.map.mapping_item import MappingItem
from xplpc.message.request import Request
from xplpc.serializer.json_serializer import JsonSerializer


@pytest.fixture(scope="session", autouse=True)
def setup_and_teardown_session():
    logging.basicConfig(level=logging.DEBUG)

    yield


@pytest.fixture(autouse=True)
def setup_and_teardown():
    yield

    # A call left waiting is the defect this project has found most often, so every test is held to leaving none.
    # The tests here bring the library up themselves, so one that ran before any of them has nothing bound to ask.
    if Layer().libc:
        assert Layer().pending_count() == 0
        assert Layer().buffer_count() == 0


def test_a_mapping_that_cannot_be_declared_leaves_nothing_behind():
    # The router routes from the names it was told, so a name recorded locally but never declared would be unreachable for the life of the process with nothing reporting it.

    XPLPC().initialize(Config(JsonSerializer()))

    name = "platform.\ud800"
    before = MappingList().count()

    with pytest.raises(UnicodeEncodeError):
        MappingList().add(name, MappingItem(lambda message, r: r(None)))

    assert not MappingList().has(name)
    assert MappingList().count() == before


def test_a_call_arriving_while_a_mapping_is_declared_finds_it(monkeypatch):
    # The router can send a call the moment a name is declared, so the mapping has to exist locally before the router hears about it.

    XPLPC().initialize(Config(JsonSerializer()))

    layer = Layer()
    declare = layer.declare
    answers = []

    def declare_and_route(name):
        declare(name)
        answers.append(Client.call_sync(Request(name)))

    monkeypatch.setattr(layer, "declare", declare_and_route)

    MappingList().add("platform.declared.window", MappingItem(lambda m, r: r("found")))

    assert answers == ["found"]


def test_a_layer_that_failed_to_bind_halfway_can_be_bound_again(monkeypatch):
    # A library that loads and lacks a symbol fails partway through binding, and nothing may look bound afterwards.

    incomplete = ctypes.util.find_library("c") or ctypes.util.find_library("msvcrt")
    layer = object.__new__(Layer)
    layer._init()

    monkeypatch.setattr(layer, "get_lib_path", lambda: incomplete)

    with pytest.raises(AttributeError):
        layer.initialize()

    assert layer.libc is None
    assert layer.handle == 0
    assert layer.host is None

    with pytest.raises(AttributeError):
        layer.initialize()


def test_a_clear_racing_an_add_leaves_the_router_and_the_list_agreeing(
    monkeypatch, caplog
):
    # A clear landing between the two steps of an add would leave a name the router routes to and no mapping behind it.

    XPLPC().initialize(Config(JsonSerializer()))

    layer = Layer()
    declare = layer.declare
    declaring = threading.Event()
    release = threading.Event()
    saved = dict(MappingList().list)

    def slow_declare(name):
        declaring.set()
        release.wait(5)
        declare(name)

    monkeypatch.setattr(layer, "declare", slow_declare)

    adding = threading.Thread(
        target=lambda: MappingList().add(
            "platform.ghost", MappingItem(lambda m, r: r("ghost"))
        )
    )
    adding.start()

    assert declaring.wait(5)

    clearing = threading.Thread(target=MappingList().clear)
    clearing.start()
    clearing.join(0.2)

    release.set()

    adding.join()
    clearing.join()

    monkeypatch.setattr(layer, "declare", declare)

    try:
        with caplog.at_level(logging.ERROR, logger="XPLPC"):
            assert Client.call_sync(Request("platform.ghost")) is None

        assert not MappingList().has("platform.ghost")
        assert not any(
            "Mapping not found" in record.getMessage() for record in caplog.records
        )
    finally:
        for name, item in saved.items():
            MappingList().add(name, item)
