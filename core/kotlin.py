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

    if c.dependency_tool == "conan":
        tool.check_tool_conan()

    ndk_root = tool.check_and_get_env("NDK_ROOT")

    # environment
    target = "kotlin"

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

    target_data = get_target_data_for_platform("kotlin")

    build_dir = os.path.join(c.proj_path, "build", target)

    # dry run
    if not dry_run:
        f.recreate_dir(build_dir)

    # build
    for item in target_data:
        l.i(f"Building for arch {item['arch']}...")

        arch_dir = os.path.join(build_dir, item["arch"])

        run_args = [
            "cmake",
            "-S",
            ".",
            "-B",
            arch_dir,
            f"-DXPLPC_TARGET={target}",
            "-DXPLPC_ADD_CUSTOM_DATA=ON",
            f"-DCMAKE_BUILD_TYPE={build_type}",
            f"-DXPLPC_DEPENDENCY_TOOL={c.dependency_tool}",
        ]

        if interface:
            run_args.append("-DXPLPC_ENABLE_INTERFACE=ON")

        # toolchain
        toolchain_file = os.path.join(
            ndk_root, "build", "cmake", "android.toolchain.cmake"
        )
        run_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")

        if c.dependency_tool == "conan":
            run_args.append(
                "-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=build/conan/conan_provider.cmake"
            )

        r.run(run_args)

        # build
        l.i(f"Building...")
        r.run(["cmake", "--build", arch_dir])

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_sample():
    # check
    sample_dir = os.path.join("kotlin", "sample")
    tool.check_tool_gradlew(sample_dir)

    if c.dependency_tool == "conan":
        tool.check_tool_conan()

    # build
    l.i("Building...")
    util.run_gradle(["clean", "build"], sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_aar():
    # check
    lib_dir = os.path.join("kotlin", "lib")
    tool.check_tool_gradlew(lib_dir)

    if c.dependency_tool == "conan":
        tool.check_tool_conan()

    # configure
    target = "kotlin"
    l.i(f"Configuring...")

    interface = util.get_param_interface(target)
    l.i(f"Interface: {interface}")

    # build
    l.i("Building...")

    run_args = ["clean", ":library:build"]
    run_args.extend(["-P", f"xplpc_dependency_tool={c.dependency_tool}"])

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

    if c.dependency_tool == "conan":
        tool.check_tool_conan()

    # test
    l.i("Testing...")

    # unit
    util.run_gradle(
        [
            "test",
            "-P",
            f"xplpc_dependency_tool={c.dependency_tool}",
        ],
        lib_dir,
    )

    # integration
    util.run_gradle(
        [
            "connectedAndroidTest",
            "-P",
            f"xplpc_dependency_tool={c.dependency_tool}",
        ],
        lib_dir,
    )

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


# -----------------------------------------------------------------------------
def get_target_data_for_platform(platform):
    if platform == "kotlin":
        return c.targets["kotlin"]

    if platform:
        l.e(f"Invalid platform: {platform}")
    else:
        l.e(f"Define a valid platform")
