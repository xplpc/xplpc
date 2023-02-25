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
    target = "swift"
    os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # configure
    l.i(f"Configuring...")

    build_type = util.get_param_build_type(target, "cmake")
    l.i(f"Build type: {build_type}")

    interface = util.get_param_interface(target)
    l.i(f"Interface: {interface}")

    platform = util.get_param_platform(target)
    l.i(f"Platform: {platform}")
    framework_list = get_framework_list(platform)

    # build
    l.i("Building...")

    do_build(
        target=target,
        build_type=build_type,
        platform=platform,
        framework_list=framework_list,
        has_interface=interface,
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_xcframework():
    # check
    tool.check_tool_xcodebuild()

    # configure
    target = "swift"
    l.i(f"Configuring...")

    platform = util.get_param_platform(target)
    l.i(f"Platform: {platform}")
    framework_list = get_framework_list(platform)

    # build
    l.i("Building...")

    do_build_xcframework(
        target=target,
        platform=platform,
        framework_list=framework_list,
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_test():
    # check
    tool.check_tool_cmake()

    # environment
    target = "swift"
    os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # configure
    l.i(f"Configuring...")

    build_type = util.get_param_build_type(target, "cmake")
    l.i(f"Build type: {build_type}")

    # test
    l.i("Testing...")
    build_dir = os.path.join(c.proj_path, "build", f"{target}-test")
    f.recreate_dir(build_dir)

    for item in c.swift_test_list:
        l.i(f"Testing for arch {item['arch']}...")

        arch_dir = os.path.join(build_dir, item["arch"])

        r.run(
            [
                "cmake",
                "-S",
                ".",
                "-B",
                arch_dir,
                "-GXcode",
                f"-DCMAKE_BUILD_TYPE={build_type}",
                f"-DXPLPC_TARGET={target}",
                "-DXPLPC_ADD_CUSTOM_DATA=ON",
                "-DXPLPC_ENABLE_TESTS=ON",
            ]
        )

        r.run(["cmake", "--build", arch_dir, "--config", build_type])

        r.run(["ctest", "-C", build_type, "--output-on-failure"], cwd=arch_dir)

        util.show_file_contents(
            os.path.join(arch_dir, "Testing", "Temporary", "LastTest.log")
        )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_format():
    # check
    tool.check_tool_swift_formatter()

    # format
    path_list = [
        {
            "path": os.path.join(c.proj_path, "swift", "lib"),
            "patterns": ["*.swift"],
        },
        {
            "path": os.path.join(c.proj_path, "swift", "sample"),
            "patterns": ["*.swift"],
        },
        {
            "path": os.path.join(c.proj_path, "swift", "tests"),
            "patterns": ["*.swift"],
        },
    ]

    if path_list:
        l.i("Formatting Swift files...")

        util.run_format(
            path_list=path_list,
            formatter=lambda file_item: r.run(
                [
                    "swiftformat",
                    os.path.relpath(file_item),
                ],
                cwd=c.proj_path,
            ),
            ignore_path_list=[],
        )

        l.ok()
    else:
        l.i("No Swift files found to format")


# -----------------------------------------------------------------------------
def do_build(target, build_type, platform, framework_list, has_interface):
    build_dir = os.path.join(c.proj_path, "build", f"{target}-{platform}")
    f.recreate_dir(build_dir)

    for item in framework_list:
        l.i(f"Building for arch {item['arch']}...")

        arch_dir = os.path.join(build_dir, item["arch"])
        toolchain_file = os.path.join(c.proj_path, "cmake", "ios.toolchain.cmake")

        # configure
        configure_args = [
            "cmake",
            "-S",
            ".",
            "-B",
            arch_dir,
            "-GXcode",
            f"-DCMAKE_BUILD_TYPE={build_type}",
            f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}",
            f"-DXPLPC_TARGET={target}",
            "-DXPLPC_ADD_CUSTOM_DATA=ON",
            f"-DPLATFORM={item['platform']}",
            f"-DDEPLOYMENT_TARGET={item['deployment_target']}",
            f"-DCMAKE_OSX_DEPLOYMENT_TARGET={item['deployment_target']}",
            f"-DSDK_VERSION={item['sdk_version']}",
        ]

        if has_interface:
            configure_args.append("-DXPLPC_ENABLE_INTERFACE=ON")

        r.run(configure_args)

        # build
        r.run(["cmake", "--build", arch_dir, "--config", build_type])


# -----------------------------------------------------------------------------
def do_build_xcframework(target, platform, framework_list):
    build_dir_prefix = f"{target}-{platform}"

    groups = []

    for item in framework_list:
        if "group" in item:
            if item["group"] not in groups:
                groups.append(item["group"])

    # generate framework for each group
    groups_command = []
    build_dir = os.path.join(c.proj_path, "build", f"{build_dir_prefix}-group")
    f.recreate_dir(build_dir)

    for group in groups:
        l.i(f"Building for group {group}...")

        # get first framework data for current group
        base_framework_arch = None
        arch_dir = None

        for item in framework_list:
            if item["group"] == group:
                base_framework_arch = item["arch"]
                arch_dir = os.path.join(c.proj_path, "build", build_dir_prefix, item["arch"])

        if not base_framework_arch:
            l.e(f"Base group framework was not found: {group}")

        # copy base framework
        framework_dir = os.path.join(arch_dir, "lib", "xplpc.framework")
        group_framework_dir = os.path.join(build_dir, group, "xplpc.framework")

        f.copy_all(
            framework_dir,
            group_framework_dir,
        )

        # generate single framework for group
        lipo_archs_args = []

        for item in framework_list:
            if item["group"] == group:
                arch_dir = os.path.join(c.proj_path, "build", build_dir_prefix, item["arch"])

                lipo_archs_args.append(
                    os.path.join(arch_dir, "lib", "xplpc.framework", "xplpc")
                )

        lipo_args = [
            "lipo",
            "-create",
            "-output",
        ]

        if f.dir_exists(
            os.path.join(
                group_framework_dir,
                "Versions",
            )
        ):
            lipo_args.extend(
                [
                    os.path.join(group_framework_dir, "Versions", "A", "xplpc"),
                ]
            )
        else:
            lipo_args.extend(
                [
                    os.path.join(group_framework_dir, "xplpc"),
                ]
            )

        lipo_args.extend(lipo_archs_args)
        r.run(lipo_args, cwd=c.proj_path)

        # add final framework to group
        groups_command.append("-framework")
        groups_command.append(group_framework_dir)

    # generate xcframework
    xcframework_dir = os.path.join(
        c.proj_path, "build", f"{build_dir_prefix}-xcframework", "xplpc.xcframework"
    )

    f.remove_dir(xcframework_dir)

    xcodebuild_command = ["xcodebuild", "-create-xcframework"]
    xcodebuild_command += groups_command
    xcodebuild_command += ["-output", xcframework_dir]

    r.run(xcodebuild_command, cwd=c.proj_path, silent=True)

    l.i(f"The xcframework was generated here: {xcframework_dir}")


# -----------------------------------------------------------------------------
def get_framework_list(platform):
    if platform == "ios":
        return c.swift_framework_list_for_ios
    elif platform == "macos":
        return c.swift_framework_list_for_macos
    elif platform == "ios-flutter":
        return c.swift_framework_list_for_ios_flutter
    elif platform == "macos-flutter":
        return c.swift_framework_list_for_macos_flutter

    if platform:
        l.e(f"Invalid platform: {platform}")
    else:
        l.e(f"Define a valid platform")
