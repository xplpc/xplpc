import os

from pygemstones.io import file as f
from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import config as c
from core import net, tool


# -----------------------------------------------------------------------------
def run_task_build():
    # check
    tool.check_tool_emsdk()

    # environment
    os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # build
    l.i("Building...")
    build_dir = os.path.join(c.proj_path, "build", "wasm")
    f.recreate_dir(build_dir)

    toolchain_file = os.path.join(
        os.environ.get("EMSDK"),
        "upstream",
        "emscripten",
        "cmake",
        "Modules",
        "Platform",
        "Emscripten.cmake",
    )

    r.run(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            build_dir,
            "-DXPLPC_TARGET=wasm",
            "-DXPLPC_ADD_CUSTOM_DATA=ON",
            f"-DCMAKE_BUILD_TYPE={c.build_type}",
            f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}",
        ]
    )

    r.run(["cmake", "--build", build_dir])

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_sample():
    # check
    tool.check_tool_npm()
    tool.check_tool_node()

    # environment
    os.environ["BASE_URL"] = c.wasm_base_url

    # paths
    sample_dir = os.path.join(c.proj_path, "wasm", "sample")

    # dependencies
    l.i("Installing dependencies...")
    r.run(["npm", "install"], cwd=sample_dir)

    # build
    l.i("Building...")
    r.run(["npm", "run", "build"], cwd=sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_run_sample():
    # check
    tool.check_tool_npm()
    tool.check_tool_node()

    # paths
    sample_dir = os.path.join(c.proj_path, "wasm", "sample")

    # dependencies
    l.i("Installing dependencies...")
    r.run(["npm", "install"], cwd=sample_dir)

    # build
    l.i("Building...")
    r.run(["npm", "run", "dev"], cwd=sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_serve_sample():
    dist_dir = os.path.join(c.proj_path, "wasm", "sample", "dist")
    net.serve(dist_dir)
    l.ok()


# -----------------------------------------------------------------------------
def run_task_test():
    # check
    tool.check_tool_npm()

    # paths
    sample_dir = os.path.join(c.proj_path, "wasm", "sample")

    # dependencies
    l.i("Installing dependencies...")
    r.run(["npm", "install"], cwd=sample_dir)

    # test
    l.i("Testing...")
    r.run(["npm", "run", "test:unit"], cwd=sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_format():
    # check
    tool.check_tool_npm()

    # paths
    sample_dir = os.path.join(c.proj_path, "wasm", "sample")

    # format js/css/html
    l.i("Formatting Web files...")
    r.run(["npm", "install"], cwd=sample_dir, silent=True)
    r.run(["npm", "run", "lint"], cwd=sample_dir, silent=True)

    l.ok()
