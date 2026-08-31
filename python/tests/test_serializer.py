import json
import logging
from datetime import datetime

import pytest
from xplpc.message.param import Param
from xplpc.serializer.json_serializer import JsonSerializer
from xplpc.type.dataview import DataView


@pytest.fixture(scope="session", autouse=True)
def setup_and_teardown_session():
    logging.basicConfig(level=logging.DEBUG)

    yield


def encoded_value(param):
    data = JsonSerializer().encode_request("sample.wire", [param])
    return json.loads(data)["p"][0]["v"]


def test_a_response_carrying_no_value_is_not_a_failure(caplog):
    # Five bridges answer the empty value for a document that carries no value, and the answer is the same either way, so what says whether this one agrees with them is whether it reports a failure.

    serializer = JsonSerializer()

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        assert serializer.decode_function_return_value("") is None
        assert serializer.decode_function_return_value("{}") is None
        assert serializer.decode_function_return_value('{"r":null}') is None
        assert serializer.decode_function_return_value('{"other":1}') is None
        assert serializer.decode_function_return_value('{"r":"text"}') == "text"

    assert caplog.records == []

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        assert serializer.decode_function_return_value("not-a-document") is None

    assert len(caplog.records) == 1


def test_the_wire_carries_the_character_rather_than_an_escape():
    # Every other bridge writes the character, and escaping it costs three bytes where one would do on a transport whose whole point is moving bytes.

    serializer = JsonSerializer()

    data = serializer.encode_request("f", [Param("v", "ção 😀")])

    assert data == '{"f":"f","p":[{"n":"v","v":"ção 😀"}]}'
    assert serializer.encode_function_return_value("ção") == '{"r":"ção"}'


def test_a_parameter_without_a_value_does_not_reject_the_request():
    # The abi is public, so a parameter can arrive without the key at all, and every other bridge reads that as the null the format already carries.

    serializer = JsonSerializer()

    without_value = serializer.decode_request('{"f":"x","p":[{"n":"a"}]}')
    assert without_value is not None
    assert without_value.message.get("a") is None

    explicit_null = serializer.decode_request('{"f":"x","p":[{"n":"a","v":null}]}')
    assert explicit_null is not None
    assert explicit_null.message.get("a") is None

    # A parameter with no name cannot be read by anything, so that one is still refused.
    assert serializer.decode_request('{"f":"x","p":[{"v":1}]}') is None


def test_a_value_of_another_type_is_rejected_and_said_so(caplog):
    # Answering nothing without saying why leaves a caller unable to tell a value that was absent from one that was refused.

    serializer = JsonSerializer()

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        assert serializer.decode_function_return_value('{"r":42}', str) is None

    assert len(caplog.records) == 1

    caplog.clear()

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        assert serializer.decode_function_return_value('{"r":"text"}', str) == "text"

    assert caplog.records == []


def test_a_function_name_that_is_not_text_is_not_a_function_name():
    # The abi is public, so the document is not this side's to trust, and a name that is not text reaches the proxy and raises there instead of being reported.

    serializer = JsonSerializer()

    assert serializer.decode_request('{"f":123,"p":[]}').function_name == ""
    assert serializer.decode_request('{"f":["a"],"p":[]}').function_name == ""
    assert serializer.decode_request('{"f":{"a":1},"p":[]}').function_name == ""
    assert serializer.decode_request('{"f":"ok","p":[]}').function_name == "ok"


def test_the_wire_carries_no_insignificant_whitespace():
    # The reference format is what the c++ core writes, and python is the only json library here that spaces its output by default.

    data = JsonSerializer().encode_request(
        "sample.wire", [Param("a", 1), Param("b", "x")]
    )

    assert data == '{"f":"sample.wire","p":[{"n":"a","v":1},{"n":"b","v":"x"}]}'
    assert JsonSerializer().encode_function_return_value("ok") == '{"r":"ok"}'


def test_a_request_without_parameters_decodes_to_an_empty_message():
    # The reference reads a missing parameter array as no parameters rather than as a broken request.

    serializer = JsonSerializer()

    assert serializer.decode_request('{"f":"sample.wire","p":null}') is not None
    assert serializer.decode_request('{"f":"sample.wire"}') is not None
    assert serializer.decode_request('{"f":"sample.wire","p":[]}') is not None


def test_a_request_without_parameters_carries_an_empty_array():
    # The format says p is an array, and writing null there is a request no bridge can read.

    encoded = json.loads(JsonSerializer().encode_request("sample.wire", None))

    assert encoded["p"] == []


def test_data_view_travels_as_pointer_and_size():
    # The reference format is what the c++ core writes, and every bridge has to agree with it.

    value = encoded_value(Param("dataView", DataView(128, 4)))

    assert value["ptr"] == 128
    assert value["size"] == 4


def test_unsupported_value_answers_empty():
    # This bridge carries no character and no date type, so a value it cannot represent is reported instead of written wrong.

    serializer = JsonSerializer()

    assert serializer.encode_request("f", [Param("date", datetime(1985, 9, 10))]) == ""


def test_integer_keeps_its_precision():
    assert encoded_value(Param("int64", 9007199254740993)) == 9007199254740993


def test_null_value_is_preserved():
    assert encoded_value(Param("value", None)) is None


def test_invalid_data_answers_empty():
    # Every bridge answers the empty value for its type when the data cannot be read.

    serializer = JsonSerializer()

    assert serializer.decode_request("not-a-json") is None
    assert serializer.decode_request("{}").function_name == ""
    assert serializer.decode_function_return_value("not-a-json") is None


def test_a_number_that_is_not_finite_is_refused(caplog):
    # The wire has no token for infinity, so writing one would put a document on it that no other bridge can read.

    serializer = JsonSerializer()

    with caplog.at_level(logging.ERROR):
        assert serializer.encode_function_return_value(float("inf")) == ""
        assert serializer.encode_function_return_value(float("nan")) == ""

    assert "Error when encode data" in caplog.text
    assert serializer.encode_function_return_value(2.5) == '{"r":2.5}'
