import platform

from pygemstones.system import platform as p
from pygemstones.util import log as l


# -----------------------------------------------------------------------------
def get_arch_path():
    arch = platform.machine().lower()

    if arch in ["armv7l", "armv7"]:
        return "arm32"

    if arch in ["aarch64", "arm64"]:
        return "arm64"

    if arch in ["i686", "x86"]:
        return "x86"

    if arch in ["x86_64", "amd64"]:
        return "x86_64"

    l.e(f"The architecture {arch} is not supported.")

    return ""


# -----------------------------------------------------------------------------
def get_lib_binary_dir():
    if p.is_windows():
        return "bin"

    return "lib"
