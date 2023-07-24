from xplpc.core.config import Config
from xplpc.serializer.json_serializer import JsonSerializer
from xplpc.core.xplpc import XPLPC


def test_core_initialize():
    serializer = JsonSerializer()
    config = Config(serializer)
    XPLPC().initialize(config)

    assert XPLPC().is_initialized()
