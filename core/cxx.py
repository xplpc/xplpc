import os

from pygemstones.io import file as f
from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import config as c
from core import tool, util


# -----------------------------------------------------------------------------
def run_task_build_static():
    # check
    tool.check_tool_cmake()

    # environment
    target = "cxx-static"

    # dependency
    if c.dependency_tool == "cpm":
        os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # configure
    l.i(f"Configuring...")

    build_type = util.get_param_build_type(target, "cmake")
    l.i(f"Build type: {build_type}")

    dry_run = util.get_param_dry()
    l.i(f"Dry run: {dry_run}")

    interface = util.get_param_interface(target)
    l.i(f"Interface: {interface}")

    build_dir = os.path.join(c.proj_path, "build", target)
    if not dry_run:
        f.recreate_dir(build_dir)

    run_args = [
        "cmake",
        "-S",
        ".",
        "-B",
        build_dir,
        f"-DXPLPC_TARGET={target}",
        "-DXPLPC_ADD_CUSTOM_DATA=ON",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DXPLPC_DEPENDENCY_TOOL={c.dependency_tool}",
    ]

    if interface:
        run_args.append(
            "-DXPLPC_ENABLE_INTERFACE=ON",
        )

    r.run(run_args)

    # build
    l.i(f"Building...")
    r.run(["cmake", "--build", build_dir])

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_shared():
    # check
    tool.check_tool_cmake()

    # environment
    target = "cxx-shared"

    # dependency
    if c.dependency_tool == "cpm":
        os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # configure
    l.i(f"Configuring...")

    build_type = util.get_param_build_type(target, "cmake")
    l.i(f"Build type: {build_type}")

    dry_run = util.get_param_dry()
    l.i(f"Dry run: {dry_run}")

    interface = util.get_param_interface(target)
    l.i(f"Interface: {interface}")

    build_dir = os.path.join(c.proj_path, "build", target)
    if not dry_run:
        f.recreate_dir(build_dir)

    run_args = [
        "cmake",
        "-S",
        ".",
        "-B",
        build_dir,
        f"-DXPLPC_TARGET={target}",
        "-DXPLPC_ADD_CUSTOM_DATA=ON",
        "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DXPLPC_DEPENDENCY_TOOL={c.dependency_tool}",
    ]

    if interface:
        run_args.append(
            "-DXPLPC_ENABLE_INTERFACE=ON",
        )

    r.run(run_args)

    # build
    l.i(f"Building...")
    r.run(["cmake", "--build", build_dir])

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_sample():
    # check
    tool.check_tool_cmake()

    # environment
    target = "cxx-static"

    # dependency
    if c.dependency_tool == "cpm":
        os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # configure
    l.i(f"Configuring...")

    build_type = util.get_param_build_type(target, "cmake")
    l.i(f"Build type: {build_type}")

    dry_run = util.get_param_dry()
    l.i(f"Dry run: {dry_run}")

    build_dir = os.path.join(c.proj_path, "build", "cxx-sample")
    if not dry_run:
        f.recreate_dir(build_dir)

    r.run(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            build_dir,
            f"-DXPLPC_TARGET={target}",
            "-DXPLPC_ENABLE_SAMPLES=ON",
            f"-DCMAKE_BUILD_TYPE={build_type}",
            f"-DXPLPC_DEPENDENCY_TOOL={c.dependency_tool}",
        ]
    )

    # build
    l.i(f"Building...")
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
    target = "cxx-static"
    os.environ["MallocStackLogging"] = "1"

    # dependency
    if c.dependency_tool == "cpm":
        os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # configure
    l.i(f"Configuring...")

    dry_run = util.get_param_dry()
    l.i(f"Dry run: {dry_run}")

    build_dir = os.path.join(c.proj_path, "build", "cxx-leaks")
    if not dry_run:
        f.recreate_dir(build_dir)

    r.run(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            build_dir,
            f"-DXPLPC_TARGET={target}",
            "-DXPLPC_ENABLE_SAMPLES=ON",
            "-DCMAKE_BUILD_TYPE=Debug",
            f"-DXPLPC_DEPENDENCY_TOOL={c.dependency_tool}",
        ]
    )

    # build
    l.i(f"Building...")
    r.run(["cmake", "--build", build_dir])

    # check leaks
    l.i(f"Checking for leaks...")
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
    target = "cxx-static"

    # dependency
    if c.dependency_tool == "cpm":
        os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # configure
    l.i(f"Configuring...")

    build_type = util.get_param_build_type(target, "cmake")
    l.i(f"Build type: {build_type}")

    dry_run = util.get_param_dry()
    l.i(f"Dry run: {dry_run}")

    build_dir = os.path.join(c.proj_path, "build", "cxx-test")
    if not dry_run:
        f.recreate_dir(build_dir)

    r.run(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            build_dir,
            f"-DXPLPC_TARGET={target}",
            "-DXPLPC_ADD_CUSTOM_DATA=ON",
            "-DXPLPC_ENABLE_TESTS=ON",
            f"-DCMAKE_BUILD_TYPE={build_type}",
            f"-DXPLPC_DEPENDENCY_TOOL={c.dependency_tool}",
        ]
    )

    # build
    l.i(f"Building...")
    r.run(["cmake", "--build", build_dir, "--config", build_type])

    # test
    l.i(f"Testing...")
    r.run(
        ["ctest", "-C", build_type, "--output-on-failure"],
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
        {
            "path": os.path.join(c.proj_path, "c"),
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
