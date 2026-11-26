import json
import logging
from dataclasses import dataclass
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
        assert serializer.decode_function_return_value("[1]") is None
        assert serializer.decode_function_return_value("42") is None
        assert serializer.decode_function_return_value('{"r":"text"}') == "text"

    assert caplog.records == []

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        assert serializer.decode_function_return_value("not-a-document") is None

    assert len(caplog.records) == 1


def test_an_answer_carries_no_whitespace_and_the_character_rather_than_an_escape():
    # The conformance cases pin the bytes of a request, and an answer is written by the same serializer through another entry point.

    serializer = JsonSerializer()

    assert serializer.encode_function_return_value("ok") == '{"r":"ok"}'
    assert serializer.encode_function_return_value("ção") == '{"r":"ção"}'


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


def test_a_request_without_parameters_carries_an_empty_array():
    # The format says p is an array, and writing null there is a request no bridge can read.

    encoded = json.loads(JsonSerializer().encode_request("sample.wire", None))

    assert encoded["p"] == []


def test_data_view_travels_as_a_buffer_handle():
    # A view carries the handle its owner registered and nothing else, so no address ever crosses the wire.

    data = JsonSerializer().encode_request(
        "sample.wire", [Param("dataView", DataView(128))]
    )

    assert data == '{"f":"sample.wire","p":[{"n":"dataView","v":{"buffer":128}}]}'


def test_a_data_view_answer_is_read_by_the_view_itself(caplog):
    # A boolean is an int here, so filling the view from its fields would hand the native side handle one.

    serializer = JsonSerializer()

    assert (
        serializer.decode_function_return_value('{"r":{"buffer":5}}', DataView).buffer
        == 5
    )

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        refused = serializer.decode_function_return_value(
            '{"r":{"buffer":true}}', DataView
        )

    assert refused.buffer == 0
    assert "is not a data view" in caplog.text


def test_unsupported_value_answers_empty():
    # This bridge carries no character and no date type, so a value it cannot represent is reported instead of written wrong.

    serializer = JsonSerializer()

    assert serializer.encode_request("f", [Param("date", datetime(1985, 9, 10))]) == ""


def test_integer_keeps_its_precision():
    assert encoded_value(Param("int64", 9007199254740993)) == 9007199254740993


def test_a_number_that_is_not_finite_is_refused(caplog):
    # The wire has no token for infinity, so writing one would put a document on it that no other bridge can read.

    serializer = JsonSerializer()

    with caplog.at_level(logging.ERROR):
        assert serializer.encode_function_return_value(float("inf")) == ""
        assert serializer.encode_function_return_value(float("nan")) == ""

    assert "Error when encode data" in caplog.text
    assert serializer.encode_function_return_value(2.5) == '{"r":2.5}'


@dataclass
class Record:
    id: int
    name: str
    done: bool = False


class RequiredArguments:
    def __init__(self, id, name):
        self.id = id
        self.name = name


class Slotted:
    __slots__ = ("id", "name")


def test_a_dataclass_with_required_fields_is_filled():
    serializer = JsonSerializer()

    record = serializer.decode_function_return_value(
        '{"r":{"id":7,"name":"valid","extra":1}}', Record
    )

    assert record == Record(7, "valid")


def test_a_class_with_required_constructor_arguments_is_filled():
    value = JsonSerializer().decode_function_return_value(
        '{"r":{"id":7,"name":"valid"}}', RequiredArguments
    )

    assert isinstance(value, RequiredArguments)
    assert value.id == 7
    assert value.name == "valid"


def test_a_class_with_slots_is_filled():
    value = JsonSerializer().decode_function_return_value(
        '{"r":{"id":7,"name":"valid"}}', Slotted
    )

    assert isinstance(value, Slotted)
    assert value.id == 7
    assert value.name == "valid"


def test_a_value_that_cannot_fill_the_class_is_reported(caplog):
    serializer = JsonSerializer()

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        assert serializer.decode_function_return_value('{"r":{"id":7}}', Record) is None

    assert "Value has another type" in caplog.text


def test_a_parameter_named_twice_rejects_the_request(caplog):
    # A map keeps only one value of a repeated name, so the request is refused rather than read as something the sender did not write.

    serializer = JsonSerializer()
    data = '{"f":"auth","p":[{"n":"token","v":"allow"},{"n":"token","v":"deny"}]}'

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        assert serializer.decode_request(data) is None

    assert 'Parameter "token" of function "auth" appears more than once' in caplog.text
    assert "allow" not in caplog.text
    assert "deny" not in caplog.text


def test_a_parameter_that_is_not_an_object_is_refused_as_every_bridge_refuses_it(
    caplog,
):
    # A parameter that is not an object carries no name, which every bridge reports the same way rather than as a document it could not read.

    serializer = JsonSerializer()

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        assert serializer.decode_request('{"f":"auth","p":["token"]}') is None

    assert len(caplog.records) == 1
    assert 'Function "auth" carries a parameter whose name is not text' in caplog.text


def test_a_number_the_wire_cannot_carry_is_not_read(caplog):
    # Every other bridge refuses these tokens and a number past what a double holds, so reading them as infinity would answer what nobody else answers.

    serializer = JsonSerializer()

    for data in ['{"r":NaN}', '{"r":Infinity}', '{"r":-Infinity}', '{"r":1e400}']:
        caplog.clear()

        with caplog.at_level(logging.ERROR, logger="XPLPC"):
            assert serializer.decode_function_return_value(data, float) is None
            assert serializer.decode_function_return_value(data) is None

        assert "Error when parse json" in caplog.text

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        assert serializer.decode_request('{"f":"auth","p":[{"n":"a","v":NaN}]}') is None


def test_an_integer_past_a_double_read_as_a_float_is_another_type(caplog):
    # Python reads an integer of any size, and converting one past what a double holds raises rather than answering.

    serializer = JsonSerializer()

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        assert (
            serializer.decode_function_return_value('{"r":' + "1" * 400 + "}", float)
            is None
        )

    assert "Value has another type" in caplog.text


def test_a_builtin_type_is_never_built_from_an_object(caplog):
    # A built in type accepts no field of an object, so filling it would answer a value the wire never carried.

    serializer = JsonSerializer()

    for class_type in [float, int, str, list, tuple]:
        caplog.clear()

        with caplog.at_level(logging.ERROR, logger="XPLPC"):
            assert (
                serializer.decode_function_return_value('{"r":{"a":1}}', class_type)
                is None
            )

        assert "Value has another type" in caplog.text
