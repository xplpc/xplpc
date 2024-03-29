import os

from pygemstones.io import file as f
from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import config as c
from core import tool


# -----------------------------------------------------------------------------
def run_task_format():
    # check
    tool.check_tool_python_formatter()

    # start
    l.i("Formating files...")

    # xplpc.py
    command = [
        "black",
        "xplpc.py",
    ]
    r.run(command)

    # core
    command = [
        "black",
        "core/",
    ]
    r.run(command)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_clear():
    l.i("Clearing...")

    f.remove_dir(os.path.join(c.proj_path, "build"))
    f.remove_file(os.path.join(c.proj_path, "CMakeUserPresets.json"))
    f.remove_file(os.path.join(c.proj_path, "CMakeLists.txt.user"))

    f.remove_dir(os.path.join(c.proj_path, "kotlin", "lib", "build"))
    f.remove_dir(os.path.join(c.proj_path, "kotlin", "lib", "library", "build"))
    f.remove_dir(os.path.join(c.proj_path, "kotlin", "lib", "library", ".cxx"))

    f.remove_dir(os.path.join(c.proj_path, "kotlin", "sample", "build"))
    f.remove_dir(os.path.join(c.proj_path, "kotlin", "sample", "app", "build"))
    f.remove_dir(os.path.join(c.proj_path, "kotlin", "sample", "app", ".cxx"))

    f.remove_dir(os.path.join(c.proj_path, "wasm", "sample", "dist"))
    f.remove_dir(os.path.join(c.proj_path, "wasm", "sample", "node_modules"))
    f.remove_file(os.path.join(c.proj_path, "wasm", "sample", "package-lock.json"))

    f.remove_dir(os.path.join(c.proj_path, "flutter", "plugin", ".dart_tool"))
    f.remove_dir(os.path.join(c.proj_path, "flutter", "plugin", "build"))
    f.remove_dir(os.path.join(c.proj_path, "flutter", "plugin", "android", "build"))
    f.remove_dir(os.path.join(c.proj_path, "flutter", "plugin", "example", "build"))
    f.remove_dir(
        os.path.join(c.proj_path, "flutter", "plugin", "example", ".dart_tool")
    )
    f.remove_dir(os.path.join(c.proj_path, "flutter", "plugin", "android", ".cxx"))
    f.remove_dir(
        os.path.join(c.proj_path, "flutter", "plugin", "example", "ios", "Pods")
    )
    f.remove_dir(
        os.path.join(c.proj_path, "flutter", "plugin", "example", "macos", "Pods")
    )

    f.remove_dir(
        os.path.join(c.proj_path, "conan", "darwin-toolchain", "test_package", "build")
    )
    f.remove_file(
        os.path.join(
            c.proj_path,
            "conan",
            "darwin-toolchain",
            "test_package",
            "CMakeUserPresets.json",
        )
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_tree():
    tool.check_tool_tree()
    r.run("tree")


# -----------------------------------------------------------------------------
def run_task_brew():
    tool.check_tool_brew()
    r.run(["brew", "bundle", "install"])
