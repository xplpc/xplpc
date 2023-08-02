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
        ndk_root = tool.check_and_get_env("ANDROID_NDK_ROOT")
    elif c.dependency_tool == "conan":
        tool.check_tool_conan()

    # environment
    target = "kotlin"

    # dependency
    if c.dependency_tool == "cpm":
        os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # configure
    l.i("Configuring...")

    build_type = util.get_param_build_type(target, "cmake")
    l.i(f"Build type: {build_type}")

    dry_run = util.get_param_dry()
    l.i(f"Dry run: {dry_run}")

    interface = util.get_param_interface(target)
    l.i(f"Interface: {interface}")

    platform = util.get_param_platform(target)
    l.i(f"Platform: {platform}")

    target_data = get_target_data_for_platform(platform)

    build_dir = os.path.join(c.proj_path, "build", f"{target}-{platform}")
    conan_build_dir = os.path.join(
        c.proj_path, "build", "conan", f"{target}-{platform}"
    )

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

        if platform in ["android", "flutter"]:
            # abi
            if c.dependency_tool == "cpm":
                abi = item["arch"]
                run_args.append(f"-DANDROID_ABI={abi}")

            # api level
            if c.dependency_tool == "cpm":
                api_level = item["api_level"]
                run_args.append(f"-DANDROID_PLATFORM={api_level}")

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
        elif platform == "desktop":
            if c.dependency_tool == "conan":
                toolchain_file = os.path.join(conan_arch_dir, "conan_toolchain.cmake")
                run_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")

            r.run(run_args)

        # build
        r.run(["cmake", "--build", arch_dir])

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_sample():
    # environment
    target = "kotlin"

    # configure
    l.i("Configuring...")

    platform = util.get_param_platform(target)
    l.i(f"Platform: {platform}")

    # check
    sample_dir = os.path.join("kotlin", get_project_by_platform(platform), "sample")
    tool.check_tool_gradlew(sample_dir)

    # build
    l.i("Building...")
    util.run_gradle(["clean", "build"], sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_aar():
    # environment
    target = "kotlin"

    # configure
    l.i("Configuring...")

    platform = util.get_param_platform(target)
    l.i(f"Platform: {platform}")

    # check
    lib_dir = os.path.join("kotlin", get_project_by_platform(platform), "lib")
    tool.check_tool_gradlew(lib_dir)

    # build
    l.i("Building...")

    run_args = ["clean", ":library:build"]
    run_args.extend(["-P", f"xplpc_platform={platform}"])
    util.run_gradle(run_args, lib_dir)

    # copy aar
    aar_dir = os.path.join(c.proj_path, "build", f"kotlin-aar-{platform}")
    f.recreate_dir(aar_dir)

    output_dir = os.path.join(lib_dir, "library", "build", "outputs", "aar")

    files = f.find_files(output_dir, "*.aar")

    for file in files:
        f.copy_file(file, os.path.join(aar_dir, os.path.basename(file)))

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_jar():
    # environment
    target = "kotlin"

    # configure
    l.i("Configuring...")

    platform = util.get_param_platform(target)
    l.i(f"Platform: {platform}")

    # check
    lib_dir = os.path.join("kotlin", get_project_by_platform(platform), "lib")
    tool.check_tool_gradlew(lib_dir)

    arch_path = util.get_arch_path()

    # build
    l.i("Building...")

    run_args = ["clean", "jar"]
    run_args.extend(["-P", f"xplpc_arch={arch_path}"])
    util.run_gradle(run_args, lib_dir)

    # copy jar
    jar_dir = os.path.join(c.proj_path, "build", f"kotlin-jar-{platform}")
    f.recreate_dir(jar_dir)

    output_dir = os.path.join(lib_dir, "build", "libs")

    files = f.find_files(output_dir, "*.jar")

    for file in files:
        f.copy_file(file, os.path.join(jar_dir, os.path.basename(file)))

    l.ok()


# -----------------------------------------------------------------------------
def run_task_test():
    # environment
    target = "kotlin"

    # configure
    l.i("Configuring...")

    platform = util.get_param_platform(target)
    l.i(f"Platform: {platform}")

    # check
    lib_dir = os.path.join("kotlin", get_project_by_platform(platform), "lib")
    tool.check_tool_gradlew(lib_dir)

    # test
    l.i("Testing...")

    # unit tests
    util.run_gradle(["test"], lib_dir)

    # integration tests
    if platform == "android":
        util.run_gradle(["connectedAndroidTest"], lib_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_run_sample():
    # environment
    target = "kotlin"

    # configure
    l.i("Configuring...")

    platform = util.get_param_platform(target)
    l.i(f"Platform: {platform}")

    # check
    sample_dir = os.path.join("kotlin", get_project_by_platform(platform), "sample")
    tool.check_tool_gradlew(sample_dir)

    # run
    l.i("Running...")

    util.run_gradle(["run"], sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_format():
    # check
    tool.check_tool_kotlin_formatter()

    # format
    path_list = [
        {
            "path": os.path.join(c.proj_path, "kotlin", "android", "lib"),
            "patterns": ["*.kt"],
        },
        {
            "path": os.path.join(c.proj_path, "kotlin", "desktop", "lib"),
            "patterns": ["*.kt"],
        },
        {
            "path": os.path.join(c.proj_path, "kotlin", "android", "sample"),
            "patterns": ["*.kt"],
        },
        {
            "path": os.path.join(c.proj_path, "kotlin", "desktop", "sample"),
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
                    "--format",
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
    if platform == "android":
        return c.targets["kotlin-android"]
    elif platform == "desktop":
        return c.targets["kotlin-desktop"]
    elif platform == "flutter":
        return c.targets["kotlin-flutter"]

    if platform:
        l.e(f"Invalid platform: {platform}")
    else:
        l.e("Define a valid platfor m")


# -----------------------------------------------------------------------------
def get_project_by_platform(platform):
    if platform == "flutter":
        return "android"
    else:
        return platform
