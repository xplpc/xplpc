from xplpc.core.config import Config
from xplpc.core.xplpc import XPLPC
from xplpc.serializer.json_serializer import JsonSerializer

serializer = JsonSerializer()
config = Config(serializer)
XPLPC().initialize(config)
