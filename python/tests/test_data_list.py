import logging
import threading

import pytest
from verified_call import verify_call
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.data.callback_list import CallbackList
from xplpc.data.mapping_list import MappingList
from xplpc.helper.byte_array_helper import ByteArrayHelper
from xplpc.map.mapping_item import MappingItem
from xplpc.message.param import Param
from xplpc.message.request import Request
from xplpc.serializer.json_serializer import JsonSerializer
from xplpc.type.dataview import DataView
from xplpc.util.unique_id import UniqueID


@pytest.fixture(scope="session", autouse=True)
def setup_and_teardown_session():
    logging.basicConfig(level=logging.DEBUG)

    XPLPC().initialize(Config(JsonSerializer()))

    yield


@pytest.fixture(autouse=True)
def setup_and_teardown_function():
    yield

    CallbackList().list.clear()
    MappingList().clear()


def test_callback_list_executes_only_once():
    calls = []

    CallbackList().add("callback-once", lambda data: calls.append(data))

    CallbackList().execute("callback-once", "")
    CallbackList().execute("callback-once", "")

    assert len(calls) == 1


def test_callback_list_removes_pending_entry():
    calls = []

    CallbackList().add("callback-removed", lambda data: calls.append(data))
    CallbackList().remove("callback-removed")
    CallbackList().execute("callback-removed", "")

    assert calls == []
    assert CallbackList().count() == 0


def test_callback_list_clears():
    CallbackList().add("a", lambda data: None)
    CallbackList().add("b", lambda data: None)

    assert CallbackList().count() == 2

    CallbackList().clear()

    assert CallbackList().count() == 0


def test_callback_list_ignores_unknown_key():
    CallbackList().execute("callback-unknown", "")

    assert CallbackList().count() == 0


def test_callback_list_is_taken_by_a_single_thread():
    calls = []
    lock = threading.Lock()

    def callback(data):
        with lock:
            calls.append(data)

    CallbackList().add("callback-concurrent", callback)

    threads = [
        threading.Thread(
            target=CallbackList().execute, args=("callback-concurrent", "")
        )
        for _ in range(16)
    ]

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

    assert len(calls) == 1


def test_mapping_list_survives_clear():
    MappingList().add("sample.temporary", MappingItem(lambda m, r: r(None)))

    assert MappingList().has("sample.temporary")

    MappingList().clear()

    assert MappingList().count() == 0
    assert not MappingList().has("sample.temporary")


def test_unique_id_generates_distinct_keys():
    keys = [UniqueID.generate() for _ in range(1000)]

    assert len(set(keys)) == 1000


def test_data_view_from_json():
    data_view = DataView.from_json({"ptr": 128, "size": 16})

    assert isinstance(data_view, DataView)
    assert data_view.ptr == 128
    assert data_view.size == 16
    assert data_view.to_json() == {"ptr": 128, "size": 16}


def test_data_view_answers_empty_for_a_value_that_is_not_a_view(caplog):
    for value in [
        {},
        {"size": 16},
        {"ptr": 128},
        {"ptr": "128", "size": 16},
        {"ptr": True, "size": 16},
        None,
    ]:
        caplog.clear()

        with caplog.at_level(logging.ERROR):
            data_view = DataView.from_json(value)

        assert data_view.ptr == 0
        assert data_view.size == 0
        assert "is not a data view" in caplog.text


def test_data_view_defaults_to_empty():
    data_view = DataView()

    assert data_view.ptr == 0
    assert data_view.size == 0


def test_native_call_proxy_carries_the_full_utf8_payload():
    received = {}

    def echo(message, r):
        received["value"] = message.get("value")
        r(message.get("value"))

    MappingList().add("test.echo.unicode", MappingItem(echo))

    # Multi byte characters make the byte length differ from the character count.
    value = "acentuação 日本語 \U0001f600"
    request = Request("test.echo.unicode", [Param("value", value)])

    assert verify_call(request) == value
    assert received["value"] == value


def test_byte_array_helper_answers_empty_for_an_empty_view():
    assert ByteArrayHelper.create_from_data_view(DataView()) == bytearray()
    assert ByteArrayHelper.create_from_data_view(DataView(0, 16)) == bytearray()
    assert ByteArrayHelper.create_from_data_view(DataView(128, 0)) == bytearray()


def test_base_serializer_cannot_be_used_without_an_implementation():
    from xplpc.serializer.base_serializer import BaseSerializer

    with pytest.raises(TypeError):
        BaseSerializer()

    class PartialSerializer(BaseSerializer):
        def decode_request(self, data):
            return None

    with pytest.raises(TypeError):
        PartialSerializer()
