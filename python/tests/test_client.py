import asyncio
import gc
import logging
import threading
import time
import weakref

import pytest
from layer_probe import invoke_layer, take_the_only_pending
from verified_call import verify_call, verify_call_from_string
from xplpc.client.client import Client
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.data.mapping_list import MappingList
from xplpc.helper.byte_array_helper import ByteArrayHelper
from xplpc.layer.layer import Layer
from xplpc.map.mapping_item import MappingItem
from xplpc.message.message import Message
from xplpc.message.param import Param
from xplpc.message.request import Request
from xplpc.message.response import Response
from xplpc.serializer.json_serializer import JsonSerializer
from xplpc.type.dataview import DataView


@pytest.fixture(scope="session", autouse=True)
def setup_and_teardown_session():
    logging.basicConfig(level=logging.DEBUG)

    serializer = JsonSerializer()
    config = Config(serializer)
    XPLPC().initialize(config)

    yield


@pytest.fixture(autouse=True)
def setup_and_teardown():
    yield

    # A call left waiting is the defect this project has found most often, so every test is held to leaving none.
    assert Layer().pending_count() == 0

    # A view never released keeps its source for the life of the process, so every test is held to releasing what it registered.
    assert Layer().buffer_count() == 0


def battery_level(m: Message, r: Response):
    suffix = m.get("suffix")
    r(f"100{suffix}")


def battery_level_async(m: Message, r: Response):
    suffix = m.get("suffix")

    def work():
        time.sleep(0.1)
        r(f"100{suffix}")

    threading.Thread(target=work, daemon=True).start()


def reverse(m: Message, r: Response):
    r("ok")


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

    response = verify_call(request)
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

    response = verify_call(request)
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

    response = verify_call(request)
    assert isinstance(response, str)


def test_reverse():
    MappingList().add(
        "platform.reverse.response",
        MappingItem(reverse),
    )

    request = Request("sample.reverse")

    response = verify_call(request)
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


def image():
    # Three opaque pixels and a transparent one, which the mapping turns into three greys and leaves the last alone.
    return bytearray([255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 0, 0, 0, 0])


def assert_grayscale(data):
    assert len(data) == 16
    assert data[0] == 85
    assert data[4] == 85
    assert data[8] == 85
    assert data[12] == 0


def test_grayscale_image_with_dataView():
    # The mapping writes through the handle into the bytes this side registered, so the source itself is what changes.

    data = image()
    data_view = DataView.create_from_byte_buffer(data)

    request = Request("sample.image.grayscale.dataview", [Param("dataView", data_view)])

    assert verify_call(request) == "OK"
    assert_grayscale(data)
    assert_grayscale(ByteArrayHelper.create_from_data_view(data_view))
    assert data_view.release()


@pytest.mark.asyncio
async def test_grayscale_image_with_dataView_async():
    data = image()
    data_view = DataView.create_from_byte_buffer(data)

    request = Request("sample.image.grayscale.dataview", [Param("dataView", data_view)])

    assert await Client.call_async(request) == "OK"
    assert_grayscale(data)
    assert data_view.release()


def assert_sample_image(data):
    assert len(data) == 16
    assert data[0] == 255
    assert data[3] == 255
    assert data[7] == 255
    assert data[12] == 0


def test_data_view():
    # A view the c++ side answers with names a buffer that side registered, so it reads here while that side keeps it.

    data_view = verify_call(Request("sample.dataview"), DataView)

    assert isinstance(data_view, DataView)
    assert data_view.buffer != 0

    original_data = ByteArrayHelper.create_from_data_view(data_view)
    assert_sample_image(original_data)

    copy = DataView.create_from_byte_buffer(original_data)

    request = Request("sample.image.grayscale.dataview", [Param("dataView", copy)])

    assert verify_call(request) == "OK"
    assert_grayscale(original_data)
    assert copy.release()


@pytest.mark.asyncio
async def test_data_view_async():
    data_view = await Client.call_async(Request("sample.dataview"), DataView)

    assert isinstance(data_view, DataView)
    assert_sample_image(ByteArrayHelper.create_from_data_view(data_view))


def test_a_view_this_side_received_cannot_be_released_here():
    # Only the side that registered a buffer can release it, so a view the c++ side answered with stays readable whatever this side asks.

    data_view = verify_call(Request("sample.dataview"), DataView)

    assert not data_view.release()
    assert_sample_image(ByteArrayHelper.create_from_data_view(data_view))


def test_a_received_view_naming_a_buffer_this_side_registered_cannot_release_it():
    # A view that comes back through the wire names the same handle, and only the view that registered it can let it go.

    data = image()
    data_view = DataView.create_from_byte_buffer(data)
    received = DataView.from_json(data_view.to_json())

    assert not received.release()
    assert ByteArrayHelper.create_from_data_view(data_view) == data
    assert data_view.release()


def test_a_released_view_reads_nothing_and_cannot_be_released_again():
    data = image()
    data_view = DataView.create_from_byte_buffer(data)

    assert ByteArrayHelper.create_from_data_view(data_view) == data
    assert data_view.release()

    assert ByteArrayHelper.create_from_data_view(data_view) == bytearray()
    assert not data_view.release()


def test_a_mapping_reading_a_released_view_answers_without_touching_the_source():
    data = image()
    data_view = DataView.create_from_byte_buffer(data)

    assert data_view.release()

    request = Request("sample.image.grayscale.dataview", [Param("dataView", data_view)])

    assert verify_call(request) == "INVALID-DATA"
    assert data == image()


def test_releasing_a_view_naming_nothing_reports_nothing(capfd):
    # A view names nothing only when making it already failed and said so, and the native side writes its lines to the process streams.

    assert not DataView().release()

    captured = capfd.readouterr()

    assert "not registered" not in captured.out + captured.err


def test_a_view_naming_nothing_reads_nothing():
    assert ByteArrayHelper.create_from_data_view(DataView()) == bytearray()
    assert ByteArrayHelper.create_from_data_view(DataView(2**63)) == bytearray()


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

    response = verify_call_from_string(request.data())

    assert '{"r":"100%"}' == response


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

    assert '{"r":"100%"}' == response


def test_data_view_pins_its_source_until_it_is_released():
    # The layer keeps the buffer exported while the handle is registered, so the source cannot move under a reader until the view is released.

    data = bytearray(b"\x01" * 16)
    data_view = DataView.create_from_byte_buffer(data)

    with pytest.raises(BufferError):
        data += b"\x02"

    assert data_view.release()

    data += b"\x02"

    assert len(data) == 17


def test_todo_round_trip():
    request = Request(
        "sample.todo.single",
        [
            Param(
                "item",
                {
                    # An object crossing the bridge exercises the nested shapes the primitives never reach.
                    "id": 1,
                    "title": "Title 1",
                    "body": "Body 1",
                    "data": {"data1": "value1"},
                    "done": True,
                },
            )
        ],
    )

    response = verify_call(request)

    assert response["id"] == 1
    assert response["title"] == "Title 1"
    assert response["data"]["data1"] == "value1"
    assert response["done"] is True


def test_echo_every_encoding_width():
    # A string has to survive every width utf8 can encode it in, on the way out and on the way back.

    samples = [
        "plain ascii",
        "caf\u00e9 na\u00efve",
        "\u4e2d\u6587\u30c6\u30b9\u30c8",
        "\U0001f600\U0001f468\u200d\U0001f469",
        "mixed \u00e9 \u4e2d \U0001f600 end",
    ]

    for sample in samples:
        response = verify_call(Request("sample.echo", [Param("value", sample)]))
        assert response == sample


def test_typed_return_value():
    # Naming the expected type has to answer the value on every bridge, not only when it is an object.

    request = Request(
        "sample.login",
        [
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", True),
        ],
    )

    assert verify_call(request, str) == "LOGGED-WITH-REMEMBER"
    assert verify_call(request, bool) is None


def test_large_data_view_crosses_without_truncating():
    # A buffer far past what a small integer holds has to cross without being truncated anywhere on the way.

    size = 4 * 1024 * 1024
    data = bytearray(b"\x64" * size)
    data_view = DataView.create_from_byte_buffer(data)

    request = Request("sample.image.grayscale.dataview", [Param("dataView", data_view)])

    assert verify_call(request) == "OK"
    assert len(ByteArrayHelper.create_from_data_view(data_view)) == size
    assert data[0] == 100
    assert data[size - 1] == 100
    assert data_view.release()


def all_types_item():
    return {
        # The edges are what a serializer loses, since a small value round trips even when the format cannot carry the type.
        "typeInt8": -128,
        "typeInt16": -32768,
        "typeInt32": -2147483648,
        "typeInt64": 9007199254740993,
        "typeFloat32": 0.1,
        "typeFloat64": 0.1,
        "typeBool": True,
        "typeOptional": None,
        "typeList": [],
        "typeMap": {"item1": "ok"},
        "typeDateTime": 494938800,
        "typeChar": 122,
        "typeChar16": 174,
        "typeChar32": 174,
        "typeWchar": 174,
        "typeString": "ok",
    }


def test_all_types_single_item_keeps_every_edge():
    item = all_types_item()
    item["typeList"] = [all_types_item()]

    response = verify_call(Request("sample.alltypes.single", [Param("item", item)]))

    assert response["typeInt8"] == -128
    assert response["typeInt16"] == -32768
    assert response["typeInt32"] == -2147483648
    assert response["typeInt64"] == 9007199254740993
    assert abs(response["typeFloat32"] - 0.1) < 1e-7
    assert response["typeFloat64"] == 0.1
    assert response["typeBool"] is True
    assert response["typeOptional"] is None
    assert len(response["typeList"]) == 1
    assert response["typeMap"]["item1"] == "ok"
    assert response["typeDateTime"] == 494938800
    assert response["typeChar"] == 122
    assert response["typeChar16"] == 174
    assert response["typeChar32"] == 174
    assert response["typeWchar"] == 174
    assert response["typeString"] == "ok"


def test_all_types_multiple_items():
    items = [all_types_item(), all_types_item()]

    response = verify_call(Request("sample.alltypes.list", [Param("items", items)]))

    assert len(response) == 2
    assert response[0]["typeInt64"] == 9007199254740993
    assert response[1]["typeString"] == "ok"


def test_todo_multiple_items():
    items = [
        {"id": 1, "title": "Title 1", "body": "Body 1", "data": {}, "done": True},
        {"id": 2, "title": "Title 2", "body": "Body 2", "data": {}, "done": True},
    ]

    response = verify_call(Request("sample.todo.list", [Param("items", items)]))

    assert len(response) == 2
    assert response[0]["title"] == "Title 1"
    assert response[1]["title"] == "Title 2"


def test_nested_host_mapping_answering_later_reaches_the_outer_caller():
    # The inner mapping holds its responder past the call, and the answer the outer call was built from still arrives once it answers.

    held = []
    MappingList().add(
        "platform.reverse.response", MappingItem(lambda m, r: held.append(r))
    )

    answered = threading.Event()
    received = []

    def callback(response):
        received.append(response)
        answered.set()

    Client.call(Request("sample.reverse"), callback, str)

    assert len(held) == 1
    assert not answered.is_set()

    threading.Thread(target=held[0], args=("ok",)).start()

    assert answered.wait(5)
    assert received == ["response-is-ok"]


def test_host_mapping_answers_after_the_call_returned():
    # A mapping that needs time answers from its own thread, which is the shape an http request takes here.

    release = threading.Event()

    def defer(m, r):
        def work():
            release.wait(5)
            r("deferred")

        threading.Thread(target=work, daemon=True).start()

    MappingList().add("platform.deferred.answer", MappingItem(defer))

    answered = threading.Event()
    received = []

    def callback(response):
        received.append(response)
        answered.set()

    Client.call(Request("platform.deferred.answer"), callback, str)

    assert received == []

    release.set()

    assert answered.wait(5)
    assert received == ["deferred"]


@pytest.mark.asyncio
async def test_call_async_gives_the_loop_a_turn_before_answering():
    # A call site has to behave the same whether the mapping is quick or slow, so the loop is reached even when the answer is already there.

    order = []

    loop = asyncio.get_running_loop()
    loop.call_soon(lambda: order.append("loop"))

    await Client.call_async(Request("sample.version"), str)
    order.append("after await")

    assert order == ["loop", "after await"]


def test_unknown_function_answers_empty_and_leaves_nothing_registered():
    # Nothing owns this name, so the caller is answered with the empty value and nothing is left waiting.

    response = verify_call(Request("not.found"))

    assert response is None
    assert Layer().pending_count() == 0


@pytest.mark.asyncio
async def test_cancelling_call_async_leaves_nothing_registered():
    # A cancelled call has to be taken back, since nothing else will ever resolve it.

    task = asyncio.create_task(Client.call_async(Request("sample.async")))

    while Layer().pending_count() == 0:
        await asyncio.sleep(0.001)

    task.cancel()

    with pytest.raises(asyncio.CancelledError):
        await task

    assert Layer().pending_count() == 0


@pytest.mark.asyncio
async def test_a_cancelled_call_async_runs_nothing_when_its_answer_arrives(
    monkeypatch, caplog
):
    # Cancelling takes the call back from the router, so the answer the mapping produces later resolves nothing and reaches nothing on this side.

    responders = []
    MappingList().add("platform.later", MappingItem(lambda m, r: responders.append(r)))

    delivered = []
    deliver = Layer()._deliver

    def spy(token, payload):
        delivered.append(token)
        deliver(token, payload)

    monkeypatch.setattr(Layer(), "_deliver", spy)

    task = asyncio.create_task(Client.call_async(Request("platform.later")))

    while not responders:
        await asyncio.sleep(0.001)

    assert len(Layer().pending) == 1

    task.cancel()

    with pytest.raises(asyncio.CancelledError):
        await task

    assert Layer().pending_count() == 0

    with caplog.at_level(logging.DEBUG, logger="XPLPC"):
        responders[0]("late")

    assert delivered == []
    assert caplog.records == []
    assert Layer().pending_count() == 0


def test_a_call_carrying_bytes_that_are_not_utf8_does_not_raise(monkeypatch, caplog):
    # The bytes the router hands over are not this side's to trust, and raising here would cross a native frame.

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        answers = invoke_layer(monkeypatch, b"\xff\xfe")

    assert answers == [(7, "")]
    assert "Unable to decode data" in caplog.text


def test_an_answer_carrying_bytes_that_are_not_utf8_reaches_the_caller_empty(
    monkeypatch, caplog
):
    # A late answer arrives as bytes this side owns, so they are released even when they cannot be read.

    from ctypes import create_string_buffer

    from xplpc.layer.layer import POST_ANSWER, PendingCall

    released = []
    received = []
    pending = PendingCall(received.append)

    monkeypatch.setattr(Layer().libc, "xplpc_free", released.append)

    with Layer().lock:
        Layer().pending[id(pending)] = pending

    bad = create_string_buffer(b"\xff\xfe", 2)

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        Layer().post_callback(None, POST_ANSWER, id(pending), bad, 2)

    assert received == [""]
    assert len(released) == 1
    assert "Unable to decode data" in caplog.text


def test_a_call_for_a_name_the_host_does_not_own_is_answered(caplog):
    # The router routes by the names this layer declared, so a name declared without a mapping behind it still has to be answered rather than left waiting.

    name = "host.name.nothing.owns"
    Layer().declare(name)

    try:
        with caplog.at_level(logging.ERROR, logger="XPLPC"):
            assert Client.call_sync(Request(name)) is None
    finally:
        Layer().retract(name)

    assert f"Mapping not found for function: {name}" in caplog.text


def test_uninitialized_library_answers_a_call_it_receives(monkeypatch):
    # The router holds this layer from the moment it attaches, so a call reaching it before the library is ready is answered rather than left waiting.

    XPLPC().initialized = False

    try:
        answers = invoke_layer(monkeypatch, b'{"f":"platform.battery.level","p":[]}')
    finally:
        XPLPC().initialized = True

    assert answers == [(7, "")]


@pytest.mark.asyncio
async def test_every_entry_point_names_the_library_when_it_was_not_initialized(caplog):
    # Reading the config or the native entry points before there are any fails on whatever is missing, which names nothing a caller can act on.

    XPLPC().initialized = False

    try:
        for source, call in [
            ("call", lambda: Client.call(Request("x.y", []), lambda d: None)),
            ("call_from_string", lambda: Client.call_from_string('{"f":"x.y","p":[]}')),
            ("call_sync", lambda: Client.call_sync(Request("x.y", []))),
            (
                "call_sync_from_string",
                lambda: Client.call_sync_from_string('{"f":"x.y","p":[]}'),
            ),
        ]:
            caplog.clear()

            with caplog.at_level(logging.ERROR):
                with pytest.raises(RuntimeError, match="XPLPC was not initialized"):
                    call()

            assert source in caplog.text

        for source, coroutine in [
            ("call_async", Client.call_async(Request("x.y", []))),
            (
                "call_async_from_string",
                Client.call_async_from_string('{"f":"x.y","p":[]}'),
            ),
        ]:
            caplog.clear()

            with caplog.at_level(logging.ERROR):
                with pytest.raises(RuntimeError, match="XPLPC was not initialized"):
                    await coroutine

            assert source in caplog.text
    finally:
        XPLPC().initialized = True


def test_a_python_mapping_declared_under_a_cxx_name_takes_it_over_until_cleared():
    # The last layer to declare a name is the one that receives it, and retracting gives the name back to the owner before it.

    request = Request(
        "sample.login",
        [
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", True),
        ],
    )
    received = []
    saved = dict(MappingList().list)

    def login(m, r):
        received.append(m.get("username"))
        r("LOGGED-BY-PYTHON")

    assert Client.call_sync(request) == "LOGGED-WITH-REMEMBER"

    MappingList().add("sample.login", MappingItem(login))

    try:
        assert Client.call_sync(request) == "LOGGED-BY-PYTHON"
        assert received == ["paulo"]
    finally:
        MappingList().clear()

        for name, item in saved.items():
            MappingList().add(name, item)

    assert Client.call_sync(request) == "LOGGED-WITH-REMEMBER"
    assert received == ["paulo"]


def test_call_sync_answers_the_value_from_an_inline_mapping():
    # A mapping that answers before it returns has already resolved the call, so the value is there to be read.

    MappingList().add("platform.battery.level", MappingItem(battery_level))

    request = Request("platform.battery.level", [Param("suffix", "%")])

    assert Client.call_sync(request) == "100%"


def test_call_sync_answers_the_value_from_an_inline_native_mapping():
    request = Request(
        "sample.login",
        [
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", True),
        ],
    )

    assert Client.call_sync(request) == "LOGGED-WITH-REMEMBER"


def test_call_sync_answers_empty_when_the_mapping_defers(monkeypatch, caplog):
    # A mapping that answers later cannot be read synchronously, so the call is taken back and the answer that arrives afterwards reaches nothing.

    release = threading.Event()
    answered = threading.Event()

    def defer(m, r):
        def work():
            release.wait(5)
            r("late")
            answered.set()

        threading.Thread(target=work, daemon=True).start()

    MappingList().add("platform.deferred.sync", MappingItem(defer))

    delivered = []
    deliver = Layer()._deliver

    def spy(token, payload):
        delivered.append(token)
        deliver(token, payload)

    monkeypatch.setattr(Layer(), "_deliver", spy)

    with caplog.at_level(logging.ERROR):
        assert Client.call_sync(Request("platform.deferred.sync")) is None

    assert "did not answer synchronously" in caplog.text
    assert Layer().pending_count() == 0

    release.set()

    assert answered.wait(5)
    assert delivered == []
    assert Layer().pending_count() == 0


def test_call_sync_answers_empty_when_nothing_owns_the_function():
    assert Client.call_sync(Request("not.found")) is None
    assert Layer().pending_count() == 0


def test_call_sync_answers_what_the_nested_mapping_produced():
    # The core reaches back for a mapping this side owns, and a nested answer produced inline is still there when the outer call returns.

    MappingList().add("platform.reverse.response", MappingItem(reverse))

    assert Client.call_sync(Request("sample.reverse")) == "response-is-ok"


def test_call_sync_answers_nothing_for_a_mismatched_type(caplog):
    # Asking for a type the answer cannot hold is answered with nothing rather than a value read the wrong way.

    request = Request(
        "sample.login",
        [
            Param("username", "paulo"),
            Param("password", "123456"),
            Param("remember", True),
        ],
    )

    with caplog.at_level(logging.ERROR):
        assert Client.call_sync(request, bool) is None

    assert "Value has another type" in caplog.text


def test_call_sync_answers_the_document_from_a_string():
    request = '{"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]}'

    assert Client.call_sync_from_string(request) == '{"r":"LOGGED-WITH-REMEMBER"}'


def test_call_sync_answers_empty_from_a_string_when_the_mapping_defers(caplog):
    release = threading.Event()
    answered = threading.Event()

    def defer(m, r):
        def work():
            release.wait(5)
            r("late")
            answered.set()

        threading.Thread(target=work, daemon=True).start()

    MappingList().add("platform.deferred.sync", MappingItem(defer))

    with caplog.at_level(logging.ERROR):
        response = Client.call_sync_from_string('{"f":"platform.deferred.sync","p":[]}')

    assert response == ""
    assert "did not answer synchronously" in caplog.text
    assert Layer().pending_count() == 0

    # The mapping answers into a call that was already taken back, and the suite waits for it rather than leaving it running past the process.
    release.set()
    assert answered.wait(5)


def test_call_sync_answers_empty_from_a_string_when_nothing_owns_the_function():
    assert Client.call_sync_from_string('{"f":"not.found","p":[]}') == ""
    assert Layer().pending_count() == 0


def test_a_request_that_cannot_be_read_is_reported_once(monkeypatch, caplog):
    # One event is one line, so the decoder says what failed and the layer answers without repeating it.

    for data in [b"not-a-json", b"null", b'{"p":[]}']:
        caplog.clear()

        with caplog.at_level(logging.ERROR):
            answers = invoke_layer(monkeypatch, data)

        errors = [r for r in caplog.records if r.levelno >= logging.ERROR]

        assert answers == [(7, "")]
        assert len(errors) == 1, f"{data!r} -> {[r.getMessage() for r in errors]}"


@pytest.mark.asyncio
async def test_a_request_carrying_text_the_wire_cannot_hold_answers_empty(caplog):
    # A path read off the filesystem carries a surrogate that utf8 cannot represent, and every entry point answers rather than raising.

    request = Request("sample.echo", [Param("value", "\ud800")])
    raw = '{"f":"sample.echo","p":[{"n":"value","v":"\ud800"}]}'

    with caplog.at_level(logging.ERROR):
        answered = []

        Client.call(request, answered.append)
        Client.call_from_string(raw, answered.append)

        assert Client.call_sync(request) is None
        assert Client.call_sync_from_string(raw) == ""
        assert await Client.call_async(request) is None
        assert await Client.call_async_from_string(raw) == ""

    assert answered == [None, ""]
    assert caplog.text.count("Error when encode the request") == 6
    assert Layer().pending_count() == 0


def test_a_callback_that_raises_on_a_valid_answer_is_called_once(caplog):
    MappingList().add("platform.once", MappingItem(lambda m, r: r("valid")))

    calls = []

    def callback(value):
        calls.append(value)

        if value is not None:
            raise ValueError("the consumer failed")

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        Client.call(Request("platform.once"), callback)

    assert calls == ["valid"]
    assert "The callback raised" in caplog.text


def test_a_serializer_that_raises_while_decoding_a_request_answers_once(monkeypatch):
    # The decode runs inside a ctypes callback, where an exception reaches nobody and the caller would wait forever.

    class RaisingSerializer(JsonSerializer):
        def decode_request(self, data):
            raise RuntimeError("the custom serializer failed")

    MappingList().add("platform.battery.level", MappingItem(battery_level))
    monkeypatch.setattr(XPLPC().config, "serializer", RaisingSerializer())

    answers = []
    Client.call(Request("platform.battery.level"), answers.append)

    assert answers == [None]


def test_a_mapping_that_answers_and_then_raises_answers_once():
    def answer_then_raise(m, r):
        r("first")
        raise RuntimeError("the mapping failed after answering")

    MappingList().add("platform.answer.raise", MappingItem(answer_then_raise))

    answers = []
    Client.call(Request("platform.answer.raise"), answers.append)

    assert answers == ["first"]


def test_a_mapping_that_raises_a_base_exception_still_answers_the_caller():
    # A ctypes callback swallows what leaves it, so a mapping raising SystemExit or KeyboardInterrupt would leave its caller waiting for good.

    def exit_inside(m, r):
        raise SystemExit(1)

    MappingList().add("platform.base.raise", MappingItem(exit_inside))

    assert verify_call(Request("platform.base.raise")) is None


def test_a_late_answer_whose_callback_raises_a_base_exception_is_reported(caplog):
    # A late answer is delivered inside a ctypes callback, which would print a SystemExit raised there rather than report it.

    held = []
    MappingList().add("platform.base.later", MappingItem(lambda m, r: held.append(r)))

    delivered = threading.Event()

    def callback(value):
        delivered.set()
        raise SystemExit(1)

    Client.call(Request("platform.base.later"), callback)

    assert len(held) == 1

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        answering = threading.Thread(target=held[0], args=("late",))
        answering.start()
        answering.join(5)

    assert delivered.is_set()
    assert "The callback raised" in caplog.text


def test_an_inline_answer_whose_callback_raises_a_base_exception_reaches_the_caller():
    # An inline answer runs the callback in the frame of the caller, which is who a SystemExit raised there is meant for.

    MappingList().add("platform.base.inline", MappingItem(lambda m, r: r("now")))

    def callback(value):
        raise SystemExit(3)

    with pytest.raises(SystemExit):
        Client.call(Request("platform.base.inline"), callback)


def test_a_mapping_that_answers_twice_answers_once():
    MappingList().add(
        "platform.answer.twice", MappingItem(lambda m, r: (r("first"), r("second")))
    )

    answers = []
    Client.call(Request("platform.answer.twice"), answers.append)

    assert answers == ["first"]


def test_a_mapping_answering_text_the_wire_cannot_hold_answers_empty(caplog):
    # A mapping answering with a name read off the filesystem carries a surrogate utf8 cannot represent, and a mapping answering from its own thread has nothing above it to catch that.

    def answer_later(m, r):
        threading.Thread(target=r, args=("\ud800",)).start()

    MappingList().add("platform.surrogate", MappingItem(answer_later))

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        assert verify_call(Request("platform.surrogate")) is None

    assert "Error when encode the answer" in caplog.text


def test_a_serializer_raising_on_a_late_answer_still_answers_the_caller(
    monkeypatch, caplog
):
    # A serializer of the caller's own raises where the mapping answers, and a mapping answering from its own thread has nothing above it to catch that.

    class RaisingEncoder(JsonSerializer):
        def encode_function_return_value(self, data):
            raise ValueError("the answer could not be written")

    def answer_later(m, r):
        threading.Thread(target=r, args=("late",)).start()

    MappingList().add("platform.encode.raises", MappingItem(answer_later))
    monkeypatch.setattr(XPLPC().config, "serializer", RaisingEncoder())

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        assert verify_call(Request("platform.encode.raises")) is None

    assert 'answer of function "platform.encode.raises"' in caplog.text


@pytest.mark.asyncio
async def test_a_serializer_raising_on_the_request_answers_every_entry_point(
    monkeypatch, caplog
):
    # A serializer of the caller's own can raise while it writes the request, and every typed entry point answers rather than handing the exception over.

    class RaisingEncoder(JsonSerializer):
        def encode_request(self, function_name, params=None):
            raise ValueError("the request could not be written")

    monkeypatch.setattr(XPLPC().config, "serializer", RaisingEncoder())
    answered = []

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        Client.call(Request("sample.login"), answered.append)

        assert Client.call_sync(Request("sample.login")) is None
        assert await Client.call_async(Request("sample.login")) is None

    assert answered == [None]
    assert caplog.text.count("Error when encode the request") == 3
    assert Layer().pending_count() == 0


def start_waiting_call(loop, function_name):
    # The coroutine is driven by hand so the test holds it suspended at its await without a task that would outlive the loop.

    coroutine = Client.call_async(Request(function_name))

    async def start():
        coroutine.send(None)

    loop.run_until_complete(start())

    return coroutine


def test_an_answer_reaching_a_cancelled_call_is_dropped_quietly(caplog):
    responders = []
    MappingList().add("platform.never", MappingItem(lambda m, r: responders.append(r)))

    loop = asyncio.new_event_loop()
    task = loop.create_task(Client.call_async(Request("platform.never")))
    loop.run_until_complete(asyncio.sleep(0))

    pending = take_the_only_pending()

    task.cancel()
    loop.run_until_complete(asyncio.gather(task, return_exceptions=True))
    loop.close()

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        pending.callback('{"r":1}')

    assert caplog.records == []

    responders[0](None)


def test_an_answer_reaching_a_closed_loop_is_dropped_and_said_so(caplog):
    responders = []
    MappingList().add("platform.never", MappingItem(lambda m, r: responders.append(r)))

    loop = asyncio.new_event_loop()
    coroutine = start_waiting_call(loop, "platform.never")

    pending = take_the_only_pending()
    loop.close()

    with caplog.at_level(logging.ERROR, logger="XPLPC"):
        pending.callback('{"r":1}')

    assert "The answer arrived after its loop was closed" in caplog.text

    coroutine.close()
    responders[0](None)


class Source(bytearray):
    # A plain bytearray cannot be referenced weakly, and this one is otherwise the same.
    pass


def test_the_layer_keeps_a_source_alive_until_its_view_is_released():
    # Only a handle crosses the wire, so the layer is what keeps the bytes behind it alive, for as long as the view is registered and no longer.

    responders = []
    MappingList().add(
        "platform.view.later", MappingItem(lambda m, r: responders.append((m, r)))
    )

    answered = threading.Event()

    def send():
        source = Source(b"\x07\x08\x09\x0a")
        view = DataView.create_from_byte_buffer(source)
        request = Request("platform.view.later", [Param("view", view)])
        Client.call(request, lambda value: answered.set())

        return view, weakref.ref(source)

    view, source_reference = send()
    gc.collect()

    assert source_reference() is not None

    message, respond = responders.pop()
    received = DataView.from_json(message.get("view"))

    assert ByteArrayHelper.create_from_data_view(received) == bytearray(
        b"\x07\x08\x09\x0a"
    )

    respond("ok")

    assert answered.wait(5)

    gc.collect()

    assert source_reference() is not None
    assert view.release()

    gc.collect()

    assert source_reference() is None
