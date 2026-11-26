import os

from pygemstones.io import file as f
from pygemstones.util import log as l

from core import config as c
from core import run, tool


# -----------------------------------------------------------------------------
def run_task_clear():
    l.i("Clearing...")

    f.remove_dir(os.path.join(c.proj_path, "build"))
    f.remove_file(os.path.join(c.proj_path, "CMakeUserPresets.json"))
    f.remove_file(os.path.join(c.proj_path, "CMakeLists.txt.user"))

    for project in ["android", "desktop"]:
        f.remove_dir(os.path.join(c.proj_path, "kotlin", project, "lib", "build"))
        f.remove_dir(os.path.join(c.proj_path, "kotlin", project, "sample", "build"))

    for module in [os.path.join("lib", "library"), os.path.join("sample", "app")]:
        f.remove_dir(os.path.join(c.proj_path, "kotlin", "android", module, "build"))

    f.remove_dir(os.path.join(c.proj_path, "wasm", "sample", "dist"))
    f.remove_dir(os.path.join(c.proj_path, "wasm", "sample", "node_modules"))

    f.remove_dir(os.path.join(c.proj_path, "flutter", "plugin", ".dart_tool"))
    f.remove_dir(os.path.join(c.proj_path, "flutter", "plugin", "build"))
    f.remove_dir(os.path.join(c.proj_path, "flutter", "plugin", "example", "build"))
    f.remove_dir(
        os.path.join(c.proj_path, "flutter", "plugin", "example", ".dart_tool")
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_brew():
    tool.check_tool_brew()
    run.run(["brew", "bundle", "install"])
