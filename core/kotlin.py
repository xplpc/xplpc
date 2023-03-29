import os

from pygemstones.io import file as f
from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import conan
from core import config as c
from core import tool, util


# -----------------------------------------------------------------------------
def run_task_build():
    # check
    tool.check_tool_cmake()

    if c.dependency_tool == "cpm":
        ndk_root = tool.check_and_get_env("NDK_ROOT")
    elif c.dependency_tool == "conan":
        tool.check_tool_conan()

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
    conan_build_dir = os.path.join(c.proj_path, "build", "conan", target)

    # dry run
    if not dry_run:
        f.recreate_dir(build_dir)

    # dependencies
    no_deps = util.get_param_no_deps()

    if not dry_run and not no_deps and c.dependency_tool == "conan":
        for item in target_data:
            l.i(f"Building dependencies for arch {item['arch']}...")

            arch_dir = os.path.join(conan_build_dir, item["arch"])
            f.recreate_dir(arch_dir)

            # conan
            build_profile = conan.get_build_profile()

            if build_profile != "default":
                build_profile = os.path.join(
                    c.proj_path, "conan", "profiles", build_profile
                )

            run_args = [
                "conan",
                "install",
                c.proj_path,
                "-pr:b",
                build_profile,
                "-pr:h",
                os.path.join(c.proj_path, "conan", "profiles", item["conan_profile"]),
            ]

            conan.add_target_setup_common_args(run_args, item, build_type)

            run_args.append("--build=missing")
            run_args.append("--update")

            r.run(run_args, cwd=arch_dir)

    # build
    for item in target_data:
        l.i(f"Building for arch {item['arch']}...")

        arch_dir = os.path.join(build_dir, item["arch"])
        conan_arch_dir = os.path.join(conan_build_dir, item["arch"])

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

        # interface
        if interface:
            run_args.append("-DXPLPC_ENABLE_INTERFACE=ON")
        else:
            run_args.append("-DXPLPC_ENABLE_INTERFACE=OFF")

        # toolchain
        if c.dependency_tool == "cpm":
            toolchain_file = os.path.join(
                ndk_root, "build", "cmake", "android.toolchain.cmake"
            )
            run_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")
        elif c.dependency_tool == "conan":
            toolchain_file = os.path.join(conan_arch_dir, "conan_toolchain.cmake")
            run_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")

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

    # unit
    util.run_gradle(["test"], lib_dir)

    # integration
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


# -----------------------------------------------------------------------------
def get_target_data_for_platform(platform):
    if platform == "kotlin":
        return c.targets["kotlin"]

    if platform:
        l.e(f"Invalid platform: {platform}")
    else:
        l.e(f"Define a valid platform")
