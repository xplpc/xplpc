from collections.abc import Callable
from typing import Any

ClientCallback = Callable[[Any | None], None]
ClientCallbackFromString = Callable[[str], None]
