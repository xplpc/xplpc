from typing import Any, Callable, Optional

ClientCallback = Callable[[Optional[Any]], None]
ClientCallbackFromString = Callable[[str], None]
