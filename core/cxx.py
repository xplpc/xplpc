import os

from pygemstones.io import file as f
from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import config as c
from core import tool, util


# -----------------------------------------------------------------------------
def run_task_build():
    # check
    tool.check_tool_cmake()

    # environment
    os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # build
    l.i("Building...")
    build_dir = os.path.join(c.proj_path, "build", "cxx")
    f.recreate_dir(build_dir)

    r.run(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            build_dir,
            f"-DCMAKE_BUILD_TYPE={c.build_type}",
        ]
    )

    r.run(["cmake", "--build", build_dir])

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_sample():
    # check
    tool.check_tool_cmake()

    # environment
    os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # build
    l.i("Building...")
    build_dir = os.path.join(c.proj_path, "build", "cxx-sample")
    f.recreate_dir(build_dir)

    r.run(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            build_dir,
            "-DXPLPC_TARGET=sample",
            f"-DCMAKE_BUILD_TYPE={c.build_type}",
        ]
    )

    r.run(["cmake", "--build", build_dir])
    l.ok()


# -----------------------------------------------------------------------------
def run_task_run_sample():
    l.i("Running...")
    build_dir = os.path.join(c.proj_path, "build", "cxx-sample")
    r.run([util.run_name("xplpc")], cwd=os.path.join(build_dir, "bin"))
    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_leaks():
    # check
    tool.check_tool_cmake()
    tool.check_tool_leaks()

    # environment
    os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")
    os.environ["MallocStackLogging"] = "1"

    # build
    l.i("Building...")
    build_dir = os.path.join(c.proj_path, "build", "cxx-leaks")
    f.recreate_dir(build_dir)

    r.run(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            build_dir,
            "-DXPLPC_TARGET=sample",
            "-DCMAKE_BUILD_TYPE=Debug",
        ]
    )

    r.run(["cmake", "--build", build_dir])

    r.run(
        [
            "leaks",
            "--atExit",
            "--list",
            "--",
            os.path.join(
                c.proj_path, "build", "cxx-leaks", "bin", util.exec_name("xplpc")
            ),
        ]
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_test():
    # check
    tool.check_tool_cmake()

    # environment
    os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # build
    l.i("Testing...")
    build_dir = os.path.join(c.proj_path, "build", "cxx-test")
    f.recreate_dir(build_dir)

    r.run(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            build_dir,
            "-DXPLPC_TARGET=cxx",
            "-DXPLPC_ADD_CUSTOM_DATA=ON",
            "-DXPLPC_ENABLE_TESTS=ON",
        ]
    )

    r.run(["cmake", "--build", build_dir, "--config", c.build_type])

    r.run(
        ["ctest", "-C", c.build_type, "--output-on-failure"],
        cwd=os.path.join(build_dir),
    )

    util.show_file_contents(
        os.path.join(build_dir, "Testing", "Temporary", "LastTest.log")
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_format():
    # check
    tool.check_tool_cxx_formatter()

    # format
    path_list = [
        {
            "path": os.path.join(c.proj_path, "cxx"),
            "patterns": ["*.cpp", "*.hpp", "*.c", "*.h", "*.m", "*.mm"],
        },
        {
            "path": os.path.join(c.proj_path, "jni"),
            "patterns": ["*.cpp", "*.hpp", "*.c", "*.h", "*.m", "*.mm"],
        },
        {
            "path": os.path.join(c.proj_path, "objc"),
            "patterns": ["*.cpp", "*.hpp", "*.c", "*.h", "*.m", "*.mm"],
        },
        {
            "path": os.path.join(c.proj_path, "wasm"),
            "patterns": ["*.cpp", "*.hpp", "*.c", "*.h", "*.m", "*.mm"],
        },
    ]

    if path_list:
        l.i("Formatting C++ files...")

        util.run_format(
            path_list=path_list,
            formatter=lambda file_item: r.run(
                [
                    "clang-format",
                    "-style",
                    "file",
                    "-i",
                    os.path.relpath(file_item),
                ],
                cwd=c.proj_path,
            ),
            ignore_path_list=[],
        )

        l.ok()
    else:
        l.i("No C++ files found to format")
