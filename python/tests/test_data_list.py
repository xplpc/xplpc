import logging

import pytest
from verified_call import verify_call
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.data.mapping_list import MappingList
from xplpc.layer.layer import Layer, PendingCall
from xplpc.map.mapping_item import MappingItem
from xplpc.message.param import Param
from xplpc.message.request import Request
from xplpc.serializer.json_serializer import JsonSerializer
from xplpc.type.dataview import DataView


@pytest.fixture(scope="session", autouse=True)
def setup_and_teardown_session():
    logging.basicConfig(level=logging.DEBUG)

    XPLPC().initialize(Config(JsonSerializer()))

    yield


@pytest.fixture(autouse=True)
def setup_and_teardown_function():
    yield

    MappingList().clear()

    assert Layer().pending_count() == 0
    assert Layer().buffer_count() == 0


def test_cancelling_a_call_that_was_answered_takes_nothing_back():
    # A call answered before the dispatch returned is no longer waiting, so there is nothing for a cancel to decide.

    answers = []
    pending = PendingCall(answers.append)

    Layer().call('{"f":"sample.version","p":[]}', pending)

    assert answers and answers[0].startswith('{"r":')
    assert not Layer().cancel(pending)
    assert Layer().pending_count() == 0


def test_cancelling_a_call_that_never_went_out_takes_nothing_back():
    assert not Layer().cancel(PendingCall(lambda response: None))


def test_mapping_list_survives_clear():
    MappingList().add("sample.temporary", MappingItem(lambda m, r: r(None)))

    assert MappingList().has("sample.temporary")

    MappingList().clear()

    assert MappingList().count() == 0
    assert not MappingList().has("sample.temporary")


def test_data_view_from_json():
    for handle in (0, 128, 2**64 - 1):
        data_view = DataView.from_json({"buffer": handle})

        assert isinstance(data_view, DataView)
        assert data_view.buffer == handle
        assert data_view.to_json() == {"buffer": handle}


def test_data_view_answers_empty_for_a_value_that_is_not_a_view(caplog):
    # A handle outside 64 bits would wrap into another one on its way to the native side, and an address is not something a view carries.

    for value in [
        {},
        {"ptr": 128, "size": 16},
        {"buffer": True},
        {"buffer": False},
        {"buffer": -1},
        {"buffer": 2**64},
        {"buffer": 1.0},
        {"buffer": "1"},
        {"buffer": None},
        [1],
        None,
    ]:
        caplog.clear()

        with caplog.at_level(logging.ERROR):
            data_view = DataView.from_json(value)

        assert data_view.buffer == 0
        assert "is not a data view" in caplog.text


def test_data_view_defaults_to_empty():
    assert DataView().buffer == 0
    assert DataView().to_json() == {"buffer": 0}


def test_a_call_carries_the_full_utf8_payload():
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


def test_serializer_cannot_be_used_without_an_implementation():
    from xplpc.serializer.serializer import Serializer

    with pytest.raises(TypeError):
        Serializer()

    class PartialSerializer(Serializer):
        def decode_request(self, data):
            return None

    with pytest.raises(TypeError):
        PartialSerializer()


def test_a_mapping_answers_with_a_view():
    data = bytearray(b"\x01\x02\x03\x04")
    data_view = DataView.create_from_byte_buffer(data)

    MappingList().add("test.view.answer", MappingItem(lambda m, r: r(data_view)))

    try:
        answered = verify_call(Request("test.view.answer"), DataView)

        assert isinstance(answered, DataView)
        assert answered.buffer == data_view.buffer
    finally:
        assert data_view.release()


def test_a_view_is_encoded_wherever_it_sits_in_a_value():
    view = DataView(7)

    request = JsonSerializer().encode_request(
        "test.views",
        [Param("views", [view]), Param("named", {"view": view})],
    )

    assert (
        request
        == '{"f":"test.views","p":[{"n":"views","v":[{"buffer":7}]},{"n":"named","v":{"view":{"buffer":7}}}]}'
    )
    assert (
        JsonSerializer().encode_function_return_value([view]) == '{"r":[{"buffer":7}]}'
    )
