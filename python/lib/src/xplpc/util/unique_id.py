import itertools


class UniqueID:
    _counter = itertools.count(1)

    @staticmethod
    def generate() -> str:
        return f"PY-{next(UniqueID._counter)}"
