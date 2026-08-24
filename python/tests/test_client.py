import asyncio
import logging
import threading
import time

import pytest
from verified_call import verify_call, verify_call_from_string
from xplpc.client.client import Client
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.data.callback_list import CallbackList
from xplpc.data.mapping_list import MappingList
from xplpc.helper.byte_array_helper import ByteArrayHelper
from xplpc.map.mapping_item import MappingItem
from xplpc.message.message import Message
from xplpc.message.param import Param
from xplpc.message.request import Request
from xplpc.message.response import Response
from xplpc.proxy.platform_proxy import PlatformProxy
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

    # A leaked callback is the defect this project has found most often, so every test is held to leaving none.
    assert CallbackList().count() == 0


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


def raise_assertion_error(message):
    pytest.fail(message)


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


def test_grayscale_image_with_dataView():
    data = bytearray(
        [
            255,
            0,
            0,
            255,
            0,
            255,
            0,
            255,
            0,
            0,
            255,
            255,
            0,
            0,
            0,
            0,
        ]
    )

    data_view = DataView.create_from_byte_buffer(data, len(data))

    request = Request(
        "sample.image.grayscale.dataview",
        [
            Param("dataView", data_view),
        ],
    )

    response = verify_call(request)
    assert response == "OK"

    data = ByteArrayHelper.create_from_data_view(data_view)

    assert 16 == data_view.size
    assert data[0] == 85
    assert data[4] == 85
    assert data[8] == 85
    assert data[12] == 0


@pytest.mark.asyncio
async def test_grayscale_image_with_dataView_async():
    data = bytearray(
        [
            255,
            0,
            0,
            255,
            0,
            255,
            0,
            255,
            0,
            0,
            255,
            255,
            0,
            0,
            0,
            0,
        ]
    )

    data_view = DataView.create_from_byte_buffer(data, len(data))

    request = Request(
        "sample.image.grayscale.dataview",
        [
            Param("dataView", data_view),
        ],
    )

    response = await Client.call_async(request)
    assert response == "OK"

    data = ByteArrayHelper.create_from_data_view(data_view)

    assert 16 == data_view.size
    assert data[0] == 85
    assert data[4] == 85
    assert data[8] == 85
    assert data[12] == 0


def test_data_view():
    request = Request("sample.dataview")
    data_view = verify_call(request, DataView)

    assert data_view is not None

    original_data = ByteArrayHelper.create_from_data_view(data_view)

    assert len(original_data) == 16
    assert original_data[0] == 255
    assert original_data[3] == 255
    assert original_data[7] == 255
    assert original_data[12] == 0

    data_view2 = DataView.create_from_byte_buffer(original_data, len(original_data))

    request2 = Request(
        "sample.image.grayscale.dataview",
        [
            Param("dataView", data_view2),
        ],
    )

    response2 = verify_call(request2)
    assert response2 == "OK"

    processed_data = ByteArrayHelper.create_from_data_view(data_view2)

    assert data_view2.size == 16
    assert processed_data[0] == 85
    assert processed_data[4] == 85
    assert processed_data[8] == 85
    assert processed_data[12] == 0

    assert len(original_data) == 16
    assert original_data[0] == 85
    assert original_data[4] == 85
    assert original_data[8] == 85
    assert original_data[12] == 0


@pytest.mark.asyncio
async def test_data_view_async():
    request = Request("sample.dataview")
    data_view = await Client.call_async(request, DataView)

    assert data_view is not None

    original_data = ByteArrayHelper.create_from_data_view(data_view)

    assert len(original_data) == 16
    assert original_data[0] == 255
    assert original_data[3] == 255
    assert original_data[7] == 255
    assert original_data[12] == 0

    data_view2 = DataView.create_from_byte_buffer(original_data, len(original_data))

    request2 = Request(
        "sample.image.grayscale.dataview",
        [
            Param("dataView", data_view2),
        ],
    )

    response2 = await Client.call_async(request2)
    assert response2 == "OK"

    processed_data = ByteArrayHelper.create_from_data_view(data_view2)

    assert data_view2.size == 16
    assert processed_data[0] == 85
    assert processed_data[4] == 85
    assert processed_data[8] == 85
    assert processed_data[12] == 0

    assert len(original_data) == 16
    assert original_data[0] == 85
    assert original_data[4] == 85
    assert original_data[8] == 85
    assert original_data[12] == 0


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


def test_data_view_pins_its_source():
    # The view keeps the buffer exported, so the source cannot move or be released while the address is in use.

    data = bytearray(b"\x01" * 16)
    data_view = DataView.create_from_byte_buffer(data, len(data))

    with pytest.raises(BufferError):
        data += b"\x02"

    assert data_view.size == 16
    assert data_view.ptr != 0


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
    data_view = DataView.create_from_byte_buffer(data, len(data))

    request = Request("sample.image.grayscale.dataview", [Param("dataView", data_view)])

    assert verify_call(request) == "OK"
    assert data_view.size == size
    assert data[0] == 100
    assert data[size - 1] == 100


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


def deferred_answer(m, r):
    def work():
        time.sleep(0.05)
        r("deferred")

    threading.Thread(target=work, daemon=True).start()


def deferred_reverse(m, r):
    def work():
        time.sleep(0.05)
        r("ok")

    threading.Thread(target=work, daemon=True).start()


def test_nested_host_mapping_answering_later_reaches_the_outer_caller():
    # The inner mapping performs its work off thread, and the answer the outer call was built from still arrives.

    MappingList().add("platform.reverse.response", MappingItem(deferred_reverse))

    answered = threading.Event()
    received = []

    def callback(response):
        received.append(response)
        answered.set()

    Client.call(Request("sample.reverse"), callback, str)

    assert answered.wait(5)
    assert received == ["response-is-ok"]


def test_host_mapping_answers_after_the_call_returned():
    # A mapping that needs time answers from its own thread, which is the shape an http request takes here.

    MappingList().add(
        "platform.deferred.answer",
        MappingItem(deferred_answer),
    )

    answered = threading.Event()
    received = []

    def callback(response):
        received.append(response)
        answered.set()

    Client.call(Request("platform.deferred.answer"), callback, str)

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
    # Nothing owns this name, so the caller is answered with the empty value and the registration is not left behind.

    before = CallbackList().count()

    response = verify_call(Request("not.found"))

    assert response is None
    assert CallbackList().count() == before


def test_a_dispatch_that_cannot_reach_the_native_side_leaves_nothing_registered():
    # A key is taken before the native side is reached, so a failure there has to drop it rather than wait forever.

    before = CallbackList().count()
    proxy = PlatformProxy()
    original = proxy.call_native_proxy

    def raising(key, data):
        raise RuntimeError("the native side is out of reach")

    proxy.call_native_proxy = raising

    try:
        with pytest.raises(RuntimeError):
            verify_call(Request("platform.battery.level", [Param("suffix", "%")]))
    finally:
        proxy.call_native_proxy = original

    assert CallbackList().count() == before


@pytest.mark.asyncio
async def test_cancelling_call_async_leaves_nothing_registered():
    # A cancelled call has to drop its registration, since nothing else will ever resolve that key.

    before = CallbackList().count()

    task = asyncio.create_task(Client.call_async(Request("sample.async")))

    while CallbackList().count() == before:
        await asyncio.sleep(0.001)

    task.cancel()

    with pytest.raises(asyncio.CancelledError):
        await task

    assert CallbackList().count() == before


def test_a_call_carrying_bytes_that_are_not_utf8_does_not_raise():
    # The abi is public, so the buffers a host hands over are not this side's to trust, and raising here would cross a native frame.

    proxy = PlatformProxy()
    original = proxy.call_native_proxy_callback
    answers = []
    proxy.call_native_proxy_callback = lambda key, data: answers.append((key, data))

    try:
        key = b"PY-valid-key"
        bad = b"\xff\xfe"

        proxy.on_native_proxy_call(bad, len(bad), bad, len(bad))
        assert answers == []

        proxy.on_native_proxy_call(key, len(key), bad, len(bad))
        assert answers == [("PY-valid-key", "")]
    finally:
        proxy.call_native_proxy_callback = original


def test_a_call_for_a_name_the_host_does_not_own_is_answered():
    # The native side routes by a set the host declared, so a name dropped between the routing and the call still has to be answered rather than left waiting.

    proxy = PlatformProxy()
    original = proxy.call_native_proxy_callback
    answers = []

    def capture(key, data):
        answers.append((key, data))

    proxy.call_native_proxy_callback = capture

    try:
        key = b"PY-not-owned"
        data = b'{"f":"host.name.nothing.owns","p":[]}'
        proxy.on_native_proxy_call(key, len(key), data, len(data))
    finally:
        proxy.call_native_proxy_callback = original

    assert answers == [("PY-not-owned", "")]


def test_uninitialized_library_answers_a_call_it_receives():
    # The library holds the callbacks from the moment it loads, so a call reaching the host before it is ready is answered rather than left waiting.

    from xplpc.proxy.platform_proxy import PlatformProxy

    proxy = PlatformProxy()
    answered = []

    original = proxy.call_native_proxy_callback
    proxy.call_native_proxy_callback = lambda key, data: answered.append((key, data))

    XPLPC().initialized = False

    try:
        key = b"uninitialized"
        data = b'{"f":"platform.battery.level","p":[]}'

        proxy.on_native_proxy_call(key, len(key), data, len(data))

        assert answered == [("uninitialized", "")]
    finally:
        XPLPC().initialized = True
        proxy.call_native_proxy_callback = original


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


def test_call_sync_answers_the_value_from_an_inline_mapping():
    # A mapping that answers before it returns has already resolved the key, so the value is there to be read.

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


def test_call_sync_answers_empty_when_the_mapping_defers(caplog):
    # A mapping that answers later cannot be read synchronously, and the registration it would have resolved is dropped rather than left behind.

    MappingList().add("platform.battery.level", MappingItem(battery_level_async))

    request = Request("platform.battery.level", [Param("suffix", "%")])
    before = CallbackList().count()

    with caplog.at_level(logging.ERROR):
        assert Client.call_sync(request) is None

    assert "did not answer synchronously" in caplog.text
    assert CallbackList().count() == before

    # The mapping answers into a registration that is already gone, and the suite waits for it rather than leaving it running past the process.
    time.sleep(0.3)

    assert CallbackList().count() == before


def test_call_sync_answers_empty_when_nothing_owns_the_function():
    before = CallbackList().count()

    assert Client.call_sync(Request("not.found")) is None
    assert CallbackList().count() == before


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
    MappingList().add("platform.battery.level", MappingItem(battery_level_async))

    before = CallbackList().count()

    with caplog.at_level(logging.ERROR):
        response = Client.call_sync_from_string('{"f":"platform.battery.level","p":[]}')

    assert response == ""
    assert "did not answer synchronously" in caplog.text
    assert CallbackList().count() == before

    time.sleep(0.3)


def test_call_sync_answers_empty_from_a_string_when_nothing_owns_the_function():
    before = CallbackList().count()

    assert Client.call_sync_from_string('{"f":"not.found","p":[]}') == ""
    assert CallbackList().count() == before


def test_a_request_that_cannot_be_read_is_reported_once(caplog):
    # One event is one line, so the decoder says what failed and the proxy answers without repeating it.

    proxy = PlatformProxy()
    original = proxy.call_native_proxy_callback
    proxy.call_native_proxy_callback = lambda key, data: None

    try:
        for data in [b"not-a-json", b"null", b'{"p":[]}']:
            caplog.clear()

            with caplog.at_level(logging.ERROR):
                proxy.on_native_proxy_call(b"k", 1, data, len(data))

            errors = [r for r in caplog.records if r.levelno >= logging.ERROR]

            assert len(errors) == 1, f"{data!r} -> {[r.getMessage() for r in errors]}"
    finally:
        proxy.call_native_proxy_callback = original


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
    assert CallbackList().count() == 0
