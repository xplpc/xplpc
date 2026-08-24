import logging
import threading

import pytest
from verified_call import verify_call
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.data.callback_list import CallbackList
from xplpc.data.mapping_list import MappingList
from xplpc.map.mapping_item import MappingItem
from xplpc.message.param import Param
from xplpc.message.request import Request
from xplpc.serializer.json_serializer import JsonSerializer
from xplpc.util.unique_id import UniqueID

THREAD_COUNT = 16
ITERATION_COUNT = 400


@pytest.fixture(scope="session", autouse=True)
def setup_and_teardown_session():
    logging.basicConfig(level=logging.DEBUG)

    XPLPC().initialize(Config(JsonSerializer()))

    yield


# Each test starts from a clean registry, so the order they run in cannot change the result.
@pytest.fixture(autouse=True)
def setup_and_teardown_function():
    CallbackList().clear()
    MappingList().clear()

    yield

    CallbackList().clear()
    MappingList().clear()


def echo(message, r):
    r(message.get("value"))


def run_concurrently(block):
    # Every thread starts at the same instant, so the registries see the worst contention.

    start = threading.Barrier(THREAD_COUNT)

    def worker(index):
        start.wait()
        block(index)

    threads = [
        threading.Thread(target=worker, args=(index,)) for index in range(THREAD_COUNT)
    ]

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()


def test_unique_id_is_distinct_under_contention():
    # The counter is the one piece of shared state every call touches before anything else.

    keys = []
    lock = threading.Lock()

    def block(index):
        for _ in range(ITERATION_COUNT):
            key = UniqueID.generate()

            with lock:
                keys.append(key)

    run_concurrently(block)

    assert len(keys) == THREAD_COUNT * ITERATION_COUNT
    assert len(set(keys)) == len(keys)


def test_callback_list_survives_contention():
    executed = []
    lock = threading.Lock()

    def block(index):
        for i in range(ITERATION_COUNT):
            key = f"stress-{i % 32}"

            def callback(data):
                with lock:
                    executed.append(data)

            CallbackList().add(key, callback)
            CallbackList().execute(key, "data")
            CallbackList().count()

            if index % 2 == 0:
                CallbackList().remove(key)

    run_concurrently(block)

    assert len(executed) > 0


def test_mapping_list_survives_contention():
    def block(index):
        for i in range(ITERATION_COUNT):
            name = f"stress.mapping.{i % 16}"

            MappingList().add(name, MappingItem(echo))
            MappingList().find(name)
            MappingList().has(name)
            MappingList().count()

            if index == 0 and i % 64 == 0:
                MappingList().clear()

    run_concurrently(block)


def test_client_answers_every_call_from_every_thread():
    MappingList().add("stress.echo", MappingItem(echo))

    answered = []
    lock = threading.Lock()

    def block(index):
        for _ in range(32):
            request = Request("stress.echo", [Param("value", "stress")])
            response = verify_call(request)

            if response == "stress":
                with lock:
                    answered.append(response)

    run_concurrently(block)

    assert len(answered) == THREAD_COUNT * 32
    assert CallbackList().count() == 0
