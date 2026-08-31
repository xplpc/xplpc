import sys


def is_windows() -> bool:
    return sys.platform.startswith("win")


def get_lib_binary_dir() -> str:
    if is_windows():
        return "bin"

    return "lib"
