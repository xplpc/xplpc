import os

from pygemstones.io import file as f
from pygemstones.util import log as l

from core import conan, host, run, tool, util
from core import config as c


# -----------------------------------------------------------------------------
def run_task_build():
    # environment
    target = "kotlin"
    platform = util.get_param_platform()

    # check
    tool.check_tool_cmake()

    if c.dependency_tool == "cpm":
        if platform in ["android", "flutter"]:
            ndk_root = tool.check_tool_ndk()
    elif c.dependency_tool == "conan":
        tool.check_tool_conan()

    # dependency
    if c.dependency_tool == "cpm":
        os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # configure
    l.i("Configuring...")

    build_type = util.get_param_build_type(target, platform)
    l.i(f"Build type: {build_type}")

    incremental = util.get_param_incremental()
    l.i(f"Incremental: {incremental}")

    interface = util.get_param_interface()
    l.i(f"Interface: {interface}")

    l.i(f"Platform: {platform}")

    target_data = get_target_data_for_platform(platform)

    build_dir = os.path.join(c.proj_path, "build", f"{target}-{platform}")
    conan_build_dir = os.path.join(
        c.proj_path, "build", "conan", f"{target}-{platform}"
    )

    if not incremental:
        f.recreate_dir(build_dir)

    # dependencies
    no_deps = util.get_param_no_deps()

    if not incremental and not no_deps and c.dependency_tool == "conan":
        for item in target_data:
            l.i(f"Building dependencies for arch {item['arch']}...")

            arch_dir = os.path.join(conan_build_dir, item["arch"])
            f.recreate_dir(arch_dir)

            conan.install(item, build_type, arch_dir)

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
            *util.get_custom_data_args(),
            f"-DCMAKE_BUILD_TYPE={build_type}",
            f"-DXPLPC_DEPENDENCY_TOOL={c.dependency_tool}",
            f"-DXPLPC_SANITIZER={util.get_param_sanitizer()}",
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

            run.run(run_args)
        elif platform == "desktop":
            # deployment target
            if "deployment_target" in item:
                run_args.append(
                    f"-DCMAKE_OSX_DEPLOYMENT_TARGET={item['deployment_target']}"
                )

            if c.dependency_tool == "conan":
                toolchain_file = os.path.join(conan_arch_dir, "conan_toolchain.cmake")
                run_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")

            run.run(run_args)

        # build
        run.run(["cmake", "--build", arch_dir, "--config", build_type])

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_sample():
    # configure
    l.i("Configuring...")

    platform = util.get_param_platform()
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
    # configure
    l.i("Configuring...")

    platform = util.get_param_platform()
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

    if not files:
        l.e(f"The build produced no aar in {output_dir}")

    for file in files:
        f.copy_file(file, os.path.join(aar_dir, os.path.basename(file)))

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_jar():
    # configure
    l.i("Configuring...")

    platform = util.get_param_platform()
    l.i(f"Platform: {platform}")

    # check
    lib_dir = os.path.join("kotlin", get_project_by_platform(platform), "lib")
    tool.check_tool_gradlew(lib_dir)

    arch_path = host.get_arch_path()

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

    if not files:
        l.e(f"The build produced no jar in {output_dir}")

    for file in files:
        f.copy_file(file, os.path.join(jar_dir, os.path.basename(file)))

    l.ok()


# -----------------------------------------------------------------------------
def run_task_test():
    # configure
    l.i("Configuring...")

    platform = util.get_param_platform()
    l.i(f"Platform: {platform}")

    # check
    lib_dir = os.path.join("kotlin", get_project_by_platform(platform), "lib")
    tool.check_tool_gradlew(lib_dir)

    # test
    l.i("Testing...")

    # The android library carries instrumented tests alone, which run on a device, and desktop runs them on the jvm.
    task = "connectedAndroidTest" if platform == "android" else "test"
    util.run_gradle([task], lib_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_run_sample():
    # configure
    l.i("Configuring...")

    platform = util.get_param_platform()
    l.i(f"Platform: {platform}")

    # check
    sample_dir = os.path.join("kotlin", get_project_by_platform(platform), "sample")
    tool.check_tool_gradlew(sample_dir)

    # run
    l.i("Running...")

    util.run_gradle(["run"], sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_lint():
    tool.check_tool_kotlin_linter()

    l.i("Linting Kotlin files...")

    run.run(
        [
            "detekt",
            "--config",
            os.path.join(c.proj_path, "detekt.yml"),
            # The sources are named one by one, so what an earlier build left behind is never what gets read.
            "--input",
            ",".join(
                [
                    os.path.join(c.proj_path, "kotlin", "shared"),
                    os.path.join(
                        c.proj_path, "kotlin", "android", "lib", "library", "src"
                    ),
                    os.path.join(
                        c.proj_path, "kotlin", "android", "sample", "app", "src"
                    ),
                    os.path.join(c.proj_path, "kotlin", "desktop", "lib", "src"),
                    os.path.join(c.proj_path, "kotlin", "desktop", "sample", "src"),
                ]
            ),
            "--build-upon-default-config",
        ],
        cwd=c.proj_path,
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_format():
    # check
    tool.check_tool_kotlin_formatter()

    # format
    path_list = [
        {
            "path": os.path.join(c.proj_path, "kotlin", "shared"),
            "patterns": ["*.kt"],
        },
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

    l.i("Formatting Kotlin files...")

    util.run_format(
        path_list=path_list,
        formatter=lambda file_item: run.run(
            [
                "ktlint",
                os.path.relpath(file_item),
                "--format",
            ],
            cwd=c.proj_path,
        ),
    )

    l.ok()


# -----------------------------------------------------------------------------
def get_target_data_for_platform(platform):
    if platform == "android":
        return c.targets["kotlin-android"]

    # The desktop library is built for the machine the tool runs on, which is the slice the c and c++ builds declare.
    if platform == "desktop":
        return util.get_target_data_for_host()

    if platform == "flutter":
        return c.targets["kotlin-flutter"]

    l.e(f"Invalid platform: {platform}")


# -----------------------------------------------------------------------------
def get_project_by_platform(platform):
    if platform == "flutter":
        return "android"

    return platform
