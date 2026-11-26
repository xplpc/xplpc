import json
import logging
import struct
from pathlib import Path

import pytest
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.message.param import Param
from xplpc.message.request import Request
from xplpc.serializer.json_serializer import JsonSerializer
from xplpc.type.dataview import DataView

# Every bridge runs the same cases, which live at the root of the repository and are listed when the suite is collected, so a case added there needs no code here.
CONFORMANCE = Path(__file__).resolve().parents[2] / "conformance"


def case_names(group):
    return sorted(path.stem for path in (CONFORMANCE / group).glob("*.json"))


def read_case(group, name):
    return json.loads((CONFORMANCE / group / f"{name}.json").read_text("utf-8"))


@pytest.fixture(scope="module", autouse=True)
def setup_and_teardown_module():
    logging.basicConfig(level=logging.DEBUG)

    XPLPC().initialize(Config(JsonSerializer()))

    yield


# Python carries no character or date type, so a caller sends the code point or the milliseconds as an int, which the integer cases already cover.
NOT_SENT = {
    "char": "python carries no character type",
    "date": "python carries no date type",
}


def value_of(param):
    if param["type"] == "dataview":
        return DataView(param["value"])

    if param["type"] == "list<dataview>":
        return [DataView(handle) for handle in param["value"]]

    return param["value"]


@pytest.mark.parametrize("name", case_names("wire"))
def test_wire(name):
    case = read_case("wire", name)

    for param in case["params"]:
        if param["type"] in NOT_SENT:
            pytest.skip(NOT_SENT[param["type"]])

    params = [Param(param["name"], value_of(param)) for param in case["params"]]

    assert Request(case["function"], params).data() == case["bytes"]


@pytest.mark.parametrize("name", case_names("accept"))
def test_accept(name):
    case = read_case("accept", name)
    decoded = JsonSerializer().decode_request(case["document"])

    assert decoded is not None
    assert decoded.function_name == case["function"]
    assert decoded.message.data == case["params"]


@pytest.mark.parametrize("name", case_names("refuse"))
def test_refuse(name, caplog):
    case = read_case("refuse", name)

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        decoded = JsonSerializer().decode_request(case["document"])

    assert decoded is None, f"the decoder read function {decoded.function_name!r}"
    assert len(caplog.records) == 1


# Python has one integer and one floating type, so a declared width is checked on what the serializer answered, which is the width the other typed bridges declare.
WIDTHS = {
    "int8": (-(2**7), 2**7 - 1),
    "int16": (-(2**15), 2**15 - 1),
    "int32": (-(2**31), 2**31 - 1),
    "int64": (-(2**63), 2**63 - 1),
    "uint8": (0, 2**8 - 1),
    "uint16": (0, 2**16 - 1),
    "uint32": (0, 2**32 - 1),
    "uint64": (0, 2**64 - 1),
}

NOT_READ = {
    "char": "python carries no character type",
    "date": "python carries no date type",
    "list<int32>": "python reads a list without an element type",
    "list<string>": "python reads a list without an element type",
    "list<list<int32>>": "python reads a list without an element type",
    "map<int32>": "python reads a map without a value type",
    "todo": "python fills a record with whatever the wire carried",
}

FLOAT_LIMIT = 3.4028234663852886e38


def read_integer(document, width):
    value = JsonSerializer().decode_function_return_value(document, int)
    lowest, highest = width

    return value if value is not None and lowest <= value <= highest else None


def read_float(document):
    # A fraction is rounded to the nearest float, while an integer has to arrive exactly, which is the float the other typed bridges declare.
    value = JsonSerializer().decode_function_return_value(document, float)

    if value is None or abs(value) > FLOAT_LIMIT:
        return None

    written = JsonSerializer().decode_function_return_value(document)
    rounded = struct.unpack("f", struct.pack("f", value))[0]

    if isinstance(written, int) and rounded != value:
        return None

    return value


def read_value(case_type, document):
    if case_type in WIDTHS:
        return read_integer(document, WIDTHS[case_type])

    if case_type == "float":
        return read_float(document)

    if case_type == "dataview":
        view = JsonSerializer().decode_function_return_value(document, DataView)
        return None if view is None else view.buffer

    readers = {"bool": bool, "double": float, "string": str}

    assert (
        case_type in readers
    ), f"the case declares a type this runner does not know: {case_type}"

    return JsonSerializer().decode_function_return_value(document, readers[case_type])


@pytest.mark.parametrize("name", case_names("values"))
def test_values(name):
    case = read_case("values", name)

    if case["type"] in NOT_READ:
        pytest.skip(NOT_READ[case["type"]])

    value = read_value(case["type"], case["document"])

    if case["expect"] is None:
        assert value is None
        return

    assert value == case["expect"]
    assert isinstance(value, bool) == isinstance(case["expect"], bool)
