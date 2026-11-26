from collections.abc import Callable
from typing import Any

Response = Callable[[Any | None], None]
