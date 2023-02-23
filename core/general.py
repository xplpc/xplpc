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

    f.remove_dir(os.path.join(c.proj_path, "kotlin", "lib", "build"))
    f.remove_dir(os.path.join(c.proj_path, "kotlin", "lib", "library", "build"))
    f.remove_dir(os.path.join(c.proj_path, "kotlin", "lib", "library", ".cxx"))

    f.remove_dir(os.path.join(c.proj_path, "kotlin", "sample", "build"))
    f.remove_dir(os.path.join(c.proj_path, "kotlin", "sample", "app", "build"))
    f.remove_dir(os.path.join(c.proj_path, "kotlin", "sample", "app", ".cxx"))

    f.remove_dir(os.path.join(c.proj_path, "wasm", "sample", "dist"))
    f.remove_dir(os.path.join(c.proj_path, "wasm", "sample", "node_modules"))
    f.remove_file(os.path.join(c.proj_path, "wasm", "sample", "package-lock.json"))

    f.remove_file(os.path.join(c.proj_path, "flutter", "plugin", "example", "build"))

    l.ok()


# -----------------------------------------------------------------------------
def run_task_tree():
    tool.check_tool_tree()
    r.run("tree")


# -----------------------------------------------------------------------------
def run_task_brew():
    tool.check_tool_brew()
    r.run(["brew", "bundle", "install"])
