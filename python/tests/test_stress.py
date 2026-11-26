import logging
import queue
import threading

import pytest
from verified_call import verify_call
from xplpc.client.client import Client
from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.data.mapping_list import MappingList
from xplpc.layer.layer import Layer, PendingCall
from xplpc.map.mapping_item import MappingItem
from xplpc.message.param import Param
from xplpc.message.request import Request
from xplpc.serializer.json_serializer import JsonSerializer

THREAD_COUNT = 16
ITERATION_COUNT = 400
DEFERRED_COUNT = 64


@pytest.fixture(scope="session", autouse=True)
def setup_and_teardown_session():
    logging.basicConfig(level=logging.DEBUG)

    XPLPC().initialize(Config(JsonSerializer()))

    yield


# Each test starts from a clean registry, so the order they run in cannot change the result.
@pytest.fixture(autouse=True)
def setup_and_teardown_function():
    MappingList().clear()

    yield

    MappingList().clear()

    assert Layer().pending_count() == 0
    assert Layer().buffer_count() == 0


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


class Answerer:
    # A mapping that answers later hands its responder to workers that are already running, which is what makes the answer race whatever the caller does next.

    def __init__(self, worker_count=4):
        self.work = queue.Queue()
        self.workers = [
            threading.Thread(target=self._drain, daemon=True)
            for _ in range(worker_count)
        ]

        for worker in self.workers:
            worker.start()

    def mapping(self, message, r):
        self.work.put((r, message.get("value")))

    def stop(self):
        for _ in self.workers:
            self.work.put(None)

        for worker in self.workers:
            worker.join()

    def _drain(self):
        while True:
            item = self.work.get()

            if item is None:
                return

            respond, value = item
            respond(value)


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
    assert Layer().pending_count() == 0


def test_every_deferred_call_receives_its_own_answer():
    # A pending call is told apart only by the object waiting for it, so many of them waiting at once must each be answered with what they sent.

    answerer = Answerer()
    MappingList().add("stress.deferred", MappingItem(answerer.mapping))

    mismatched = []
    lock = threading.Lock()

    def block(index):
        waiting = []

        for i in range(DEFERRED_COUNT):
            sent = f"{index}-{i}"
            answered = threading.Event()
            box = {}

            def callback(value, box=box, answered=answered):
                box["value"] = value
                answered.set()

            Client.call(Request("stress.deferred", [Param("value", sent)]), callback)
            waiting.append((sent, box, answered))

        for sent, box, answered in waiting:
            assert answered.wait(5), "a deferred call was never answered"

            if box["value"] != sent:
                with lock:
                    mismatched.append((sent, box["value"]))

    try:
        run_concurrently(block)
    finally:
        answerer.stop()

    assert mismatched == []
    assert Layer().pending_count() == 0


def test_a_cancel_racing_an_answer_resolves_each_call_once():
    # Whichever of the answer and the cancel reaches the router first decides the call, and the other one resolves nothing.

    answerer = Answerer()
    MappingList().add("stress.raced", MappingItem(answerer.mapping))

    data = Request("stress.raced", [Param("value", "raced")]).data()
    outcomes = []
    lock = threading.Lock()

    def block(index):
        for _ in range(DEFERRED_COUNT):
            runs = []
            pending = PendingCall(runs.append)

            Layer().call(data, pending)
            cancelled = Layer().cancel(pending)

            with lock:
                outcomes.append((cancelled, runs))

    try:
        run_concurrently(block)
    finally:
        answerer.stop()

    for cancelled, runs in outcomes:
        assert len(runs) == (0 if cancelled else 1)

    assert len(outcomes) == THREAD_COUNT * DEFERRED_COUNT
    assert Layer().pending_count() == 0
