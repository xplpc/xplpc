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
    ndk_root = tool.check_and_get_env("NDK_ROOT")

    # environment
    target = "kotlin"
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

    toolchain_file = os.path.join(ndk_root, "build", "cmake", "android.toolchain.cmake")

    run_args = [
        "cmake",
        "-S",
        ".",
        "-B",
        build_dir,
        f"-DXPLPC_TARGET={target}",
        "-DXPLPC_ADD_CUSTOM_DATA=ON",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}",
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
    sample_dir = os.path.join("kotlin", "sample")
    tool.check_tool_gradlew(sample_dir)

    # build
    l.i("Building...")
    util.run_gradle(["clean", "build"], sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_aar():
    # check
    lib_dir = os.path.join("kotlin", "lib")
    tool.check_tool_gradlew(lib_dir)

    # configure
    target = "kotlin"
    l.i(f"Configuring...")

    interface = util.get_param_interface(target)
    l.i(f"Interface: {interface}")

    # build
    l.i("Building...")

    run_args = ["clean", ":library:build"]

    if interface:
        run_args.extend(["-P", "xplpc_interface"])

    util.run_gradle(run_args, lib_dir)

    # copy aar
    aar_dir = os.path.join(c.proj_path, "build", "kotlin-aar")
    output_dir = os.path.join(lib_dir, "library", "build", "outputs", "aar")

    files = f.find_files(output_dir, "*.aar")

    for file in files:
        f.copy_file(file, os.path.join(aar_dir, os.path.basename(file)))

    l.ok()


# -----------------------------------------------------------------------------
def run_task_test():
    # check
    lib_dir = os.path.join("kotlin", "lib")
    tool.check_tool_gradlew(lib_dir)

    # test
    l.i("Testing...")
    util.run_gradle(["test"], lib_dir)
    util.run_gradle(["connectedAndroidTest"], lib_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_format():
    # check
    tool.check_tool_kotlin_formatter()

    # format
    path_list = [
        {
            "path": os.path.join(c.proj_path, "kotlin", "lib"),
            "patterns": ["*.kt"],
        },
        {
            "path": os.path.join(c.proj_path, "kotlin", "sample"),
            "patterns": ["*.kt"],
        },
    ]

    if path_list:
        l.i("Formatting Kotlin files...")

        util.run_format(
            path_list=path_list,
            formatter=lambda file_item: r.run(
                [
                    "ktlint",
                    os.path.relpath(file_item),
                ],
                cwd=c.proj_path,
            ),
            ignore_path_list=[],
        )

        l.ok()
    else:
        l.i("No Kotlin files found to format")
