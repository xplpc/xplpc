import os

from pygemstones.io import file as f
from pygemstones.system import platform as p
from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import conan
from core import config as c
from core import tool, util


# -----------------------------------------------------------------------------
def run_task_build_static():
    # check
    tool.check_tool_cmake()

    if c.dependency_tool == "conan":
        tool.check_tool_conan()

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

    target_data = get_target_data_for_platform()

    # build
    l.i(f"Building...")

    do_build(
        target=target,
        build_type=build_type,
        target_data=target_data,
        build_folder=target,
        has_interface=interface,
        has_tests=False,
        has_samples=False,
        has_pic=False,
        has_custom_data=True,
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_shared():
    # check
    tool.check_tool_cmake()

    if c.dependency_tool == "conan":
        tool.check_tool_conan()

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

    target_data = get_target_data_for_platform()

    # build
    l.i(f"Building...")

    do_build(
        target=target,
        build_type=build_type,
        target_data=target_data,
        build_folder=target,
        has_interface=interface,
        has_tests=False,
        has_samples=False,
        has_pic=True,
        has_custom_data=True,
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_sample():
    # check
    tool.check_tool_cmake()

    if c.dependency_tool == "conan":
        tool.check_tool_conan()

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

    target_data = get_target_data_for_platform()

    # build
    l.i(f"Building...")

    do_build(
        target=target,
        build_type=build_type,
        target_data=target_data,
        build_folder="cxx-sample",
        has_interface=False,
        has_tests=False,
        has_samples=True,
        has_pic=False,
        has_custom_data=False,
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_run_sample():
    l.i("Running...")

    build_dir = os.path.join(c.proj_path, "build", "cxx-sample")

    target_data = get_target_data_for_platform()
    arch = target_data[0]["arch"]

    r.run([util.run_name("xplpc")], cwd=os.path.join(build_dir, arch, "bin"))

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_leaks():
    # check
    tool.check_tool_cmake()
    tool.check_tool_leaks()

    if c.dependency_tool == "conan":
        tool.check_tool_conan()

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

    target_data = get_target_data_for_platform()

    # build
    l.i(f"Building...")

    do_build(
        target=target,
        build_type="Debug",
        target_data=target_data,
        build_folder="cxx-leaks",
        has_interface=False,
        has_tests=False,
        has_samples=True,
        has_pic=False,
        has_custom_data=False,
    )

    # check leaks
    l.i(f"Checking for leaks...")

    arch = target_data[0]["arch"]

    r.run(
        [
            "leaks",
            "--atExit",
            "--list",
            "--",
            os.path.join(
                c.proj_path,
                "build",
                "cxx-leaks",
                arch,
                "bin",
                util.exec_name("xplpc"),
            ),
        ]
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_test():
    # check
    tool.check_tool_cmake()

    if c.dependency_tool == "conan":
        tool.check_tool_conan()

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

    target_data = get_target_data_for_platform()

    # build
    l.i(f"Building...")

    do_build(
        target=target,
        build_type=build_type,
        target_data=target_data,
        build_folder="cxx-test",
        has_interface=False,
        has_tests=True,
        has_samples=False,
        has_pic=False,
        has_custom_data=True,
    )

    # test
    l.i(f"Testing...")

    build_dir = os.path.join(c.proj_path, "build", "cxx-test")
    arch = target_data[0]["arch"]

    r.run(
        ["ctest", "-C", build_type, "--output-on-failure"],
        cwd=os.path.join(build_dir, arch),
    )

    util.show_file_contents(
        os.path.join(build_dir, arch, "Testing", "Temporary", "LastTest.log")
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


# -----------------------------------------------------------------------------
def do_build(
    target,
    build_type,
    target_data,
    build_folder,
    has_interface,
    has_tests,
    has_samples,
    has_pic,
    has_custom_data,
):
    build_dir = os.path.join(c.proj_path, "build", build_folder)
    conan_build_dir = os.path.join(c.proj_path, "build", "conan", build_folder)

    # dry run
    dry_run = util.get_param_dry()
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

            if has_tests:
                run_args.append("-o:h")
                run_args.append("xplpc_enable_tests=True")

            run_args.append("--build=missing")
            run_args.append("--update")

            r.run(run_args, cwd=arch_dir)

    # build
    for item in target_data:
        l.i(f"Building for arch {item['arch']}...")

        arch_dir = os.path.join(build_dir, item["arch"])
        conan_arch_dir = os.path.join(conan_build_dir, item["arch"])

        # configure
        run_args = [
            "cmake",
            "-S",
            ".",
            "-B",
            arch_dir,
            f"-DCMAKE_BUILD_TYPE={build_type}",
            f"-DXPLPC_TARGET={target}",
            f"-DXPLPC_DEPENDENCY_TOOL={c.dependency_tool}",
        ]

        # custom data
        if has_custom_data:
            run_args.append("-DXPLPC_ADD_CUSTOM_DATA=ON")
        else:
            run_args.append("-DXPLPC_ADD_CUSTOM_DATA=OFF")

        # interface
        if has_interface:
            run_args.append("-DXPLPC_ENABLE_INTERFACE=ON")
        else:
            run_args.append("-DXPLPC_ENABLE_INTERFACE=OFF")

        # tests
        if has_tests:
            run_args.append("-DXPLPC_ENABLE_TESTS=ON")
        else:
            run_args.append("-DXPLPC_ENABLE_TESTS=OFF")

        # sample
        if has_samples:
            run_args.append("-DXPLPC_ENABLE_SAMPLES=ON")
        else:
            run_args.append("-DXPLPC_ENABLE_SAMPLES=OFF")

        # pic
        if has_pic:
            run_args.append("-DCMAKE_POSITION_INDEPENDENT_CODE=ON")

        # toolchain
        if c.dependency_tool == "conan":
            toolchain_file = os.path.join(conan_arch_dir, "conan_toolchain.cmake")
            run_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")

        r.run(run_args)

        # build
        r.run(["cmake", "--build", arch_dir, "--config", build_type])


# -----------------------------------------------------------------------------
def get_target_data_for_platform():
    if p.is_macos():
        return c.targets["platform-macos"]
    elif p.is_windows():
        return c.targets["platform-windows"]
    elif p.is_linux():
        return c.targets["platform-linux"]
    else:
        l.e(f"Unknown platform")
