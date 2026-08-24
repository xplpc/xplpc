import os

from pygemstones.io import file as f
from pygemstones.util import log as l

from core import config as c
from core import run, tool


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
    run.run(command)

    # core
    command = [
        "black",
        "core/",
    ]
    run.run(command)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_clear():
    l.i("Clearing...")

    f.remove_dir(os.path.join(c.proj_path, "build"))
    f.remove_file(os.path.join(c.proj_path, "CMakeUserPresets.json"))
    f.remove_file(os.path.join(c.proj_path, "CMakeLists.txt.user"))

    for project in ["android", "desktop"]:
        f.remove_dir(os.path.join(c.proj_path, "kotlin", project, "lib", "build"))
        f.remove_dir(
            os.path.join(c.proj_path, "kotlin", project, "lib", "library", "build")
        )
        f.remove_dir(
            os.path.join(c.proj_path, "kotlin", project, "lib", "library", ".cxx")
        )

        f.remove_dir(os.path.join(c.proj_path, "kotlin", project, "sample", "build"))
        f.remove_dir(
            os.path.join(c.proj_path, "kotlin", project, "sample", "app", "build")
        )
        f.remove_dir(
            os.path.join(c.proj_path, "kotlin", project, "sample", "app", ".cxx")
        )

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

    l.ok()


# -----------------------------------------------------------------------------
def run_task_brew():
    tool.check_tool_brew()
    run.run(["brew", "bundle", "install"])
