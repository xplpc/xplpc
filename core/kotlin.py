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
    build_dir = os.path.join(c.proj_path, "build", "kotlin")
    f.recreate_dir(build_dir)

    toolchain_file = os.path.join(
        os.environ["NDK_ROOT"], "build", "cmake", "android.toolchain.cmake"
    )

    r.run(
        [
            "cmake",
            "-S",
            ".",
            "-B",
            build_dir,
            "-DXPLPC_TARGET=kotlin",
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

    # build
    l.i("Building...")
    util.run_gradle(["clean", ":library:build"], lib_dir)

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
