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

    if c.dependency_tool == "conan":
        tool.check_tool_conan()

    # environment
    target = "swift"

    # dependency
    if c.dependency_tool == "cpm":
        os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # configure
    l.i("Configuring...")

    build_type = util.get_param_build_type(target, format="cmake")
    l.i(f"Build type: {build_type}")

    dry_run = util.get_param_dry()
    l.i(f"Dry run: {dry_run}")

    interface = util.get_param_interface(target)
    l.i(f"Interface: {interface}")

    platform = util.get_param_platform(target)
    l.i(f"Platform: {platform}")
    target_data = get_target_data_for_platform(platform)

    # build
    l.i("Building...")

    do_build(
        target=target,
        build_type=build_type,
        platform=platform,
        target_data=target_data,
        has_interface=interface,
        has_tests=False,
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_xcframework():
    # check
    tool.check_tool_xcodebuild()

    # configure
    target = "swift"
    l.i("Configuring...")

    dry_run = util.get_param_dry()
    l.i(f"Dry run: {dry_run}")

    platform = util.get_param_platform(target)
    l.i(f"Platform: {platform}")
    target_data = get_target_data_for_platform(platform)

    # build
    l.i("Building...")

    do_build_xcframework(
        target=target,
        platform=platform,
        target_data=target_data,
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_test():
    # check
    tool.check_tool_cmake()

    # environment
    target = "swift"

    # dependency
    if c.dependency_tool == "cpm":
        os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # configure
    l.i("Configuring...")

    build_type = util.get_param_build_type(target, format="cmake")
    l.i(f"Build type: {build_type}")

    target_data = get_target_data_for_platform("test")

    # build
    l.i("Building...")

    do_build(
        target=target,
        build_type=build_type,
        platform="test",
        target_data=target_data,
        has_interface=False,
        has_tests=True,
    )

    # test
    l.i("Testing...")

    build_dir = os.path.join(c.proj_path, "build", "swift-test")
    group = target_data[0]["group"]
    arch = target_data[0]["arch"]

    r.run(
        ["ctest", "-C", build_type, "--output-on-failure"],
        cwd=os.path.join(build_dir, group, arch),
    )

    util.show_file_contents(
        os.path.join(build_dir, group, arch, "Testing", "Temporary", "LastTest.log")
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
def do_build(target, build_type, platform, target_data, has_interface, has_tests):
    build_dir = os.path.join(c.proj_path, "build", f"{target}-{platform}")
    conan_build_dir = os.path.join(
        c.proj_path, "build", "conan", f"{target}-{platform}"
    )

    # dry run
    dry_run = util.get_param_dry()
    if not dry_run:
        f.recreate_dir(build_dir)

    # dependencies
    no_deps = util.get_param_no_deps()

    if not dry_run and not no_deps and c.dependency_tool == "conan":
        for item in target_data:
            l.i(f"Building dependencies for arch {item['arch']}/{item['group']}...")

            arch_dir = os.path.join(conan_build_dir, item["group"], item["arch"])
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
        l.i(f"Building for arch {item['arch']}/{item['group']}...")

        arch_dir = os.path.join(build_dir, item["group"], item["arch"])
        conan_arch_dir = os.path.join(conan_build_dir, item["group"], item["arch"])

        # configure
        run_args = [
            "cmake",
            "-S",
            ".",
            "-B",
            arch_dir,
            "-GXcode",
            f"-DCMAKE_BUILD_TYPE={build_type}",
            f"-DXPLPC_TARGET={target}",
            "-DXPLPC_ADD_CUSTOM_DATA=ON",
            f"-DXPLPC_DEPENDENCY_TOOL={c.dependency_tool}",
            f"-DPLATFORM={item['platform']}",
            f"-DDEPLOYMENT_TARGET={item['deployment_target']}",
            f"-DCMAKE_OSX_DEPLOYMENT_TARGET={item['deployment_target']}",
            f"-DSDK_VERSION={item['sdk_version']}",
            f"-DARCHS={item['arch']}",
        ]

        # interface
        if has_interface:
            run_args.append("-DXPLPC_ENABLE_INTERFACE=ON")
        else:
            run_args.append("-DXPLPC_ENABLE_INTERFACE=OFF")

        # tests
        if has_tests:
            run_args.append("-DXPLPC_BUILD_TESTS=ON")
        else:
            run_args.append("-DXPLPC_BUILD_TESTS=OFF")

        # arc
        if "enable_arc" in item:
            run_args.append(
                "-DENABLE_ARC={0}".format("ON" if item["enable_arc"] else "OFF")
            )

        # bitcode
        if "enable_bitcode" in item:
            run_args.append(
                "-DENABLE_BITCODE={0}".format("ON" if item["enable_bitcode"] else "OFF")
            )

        # visibility
        if "enable_visibility" in item:
            run_args.append(
                "-DENABLE_VISIBILITY={0}".format(
                    "ON" if item["enable_visibility"] else "OFF"
                )
            )

        # toolchain
        if c.dependency_tool == "cpm":
            toolchain_file = os.path.join(
                c.proj_path, "cmake", "ios", "ios.toolchain.cmake"
            )
            run_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")
        elif c.dependency_tool == "conan":
            toolchain_file = os.path.join(conan_arch_dir, "conan_toolchain.cmake")
            run_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")

        r.run(run_args)

        # build
        r.run(["cmake", "--build", arch_dir, "--config", build_type])


# -----------------------------------------------------------------------------
def do_build_xcframework(target, platform, target_data):
    build_dir_prefix = f"{target}-{platform}"

    groups = []

    for item in target_data:
        if "group" in item:
            if item["group"] not in groups:
                groups.append(item["group"])

    if not groups:
        l.e("No group was defined in target data")

    # generate framework for each group
    groups_command = []
    build_dir = os.path.join(c.proj_path, "build", f"{build_dir_prefix}-group")

    dry_run = util.get_param_dry()
    if not dry_run:
        f.recreate_dir(build_dir)

    for group in groups:
        l.i(f"Building for group {group}...")

        # get first framework data for current group
        base_framework_arch = None
        arch_dir = None

        for item in target_data:
            if item["group"] == group:
                base_framework_arch = item["arch"]
                arch_dir = os.path.join(
                    c.proj_path, "build", build_dir_prefix, item["group"], item["arch"]
                )

        if not base_framework_arch:
            l.e(f"Base group framework was not found: {group}")

        # copy base framework
        framework_dir = os.path.join(arch_dir, "lib", "xplpc.framework")
        group_framework_dir = os.path.join(build_dir, group, "xplpc.framework")

        f.copy_dir(
            framework_dir,
            group_framework_dir,
            symlinks=True,
        )

        # copy swift modules
        group_framework_module_dir = os.path.join(
            group_framework_dir, "Modules", "xplpc.swiftmodule"
        )

        if f.dir_exists(group_framework_module_dir):
            for item in target_data:
                if item["group"] == group:
                    arch_dir = os.path.join(
                        c.proj_path,
                        "build",
                        build_dir_prefix,
                        item["group"],
                        item["arch"],
                    )

                    framework_module_dir = os.path.join(
                        arch_dir,
                        "lib",
                        "xplpc.framework",
                        "Modules",
                        "xplpc.swiftmodule",
                    )

                    if f.dir_exists(framework_module_dir):
                        f.copy_dir(
                            framework_module_dir,
                            group_framework_module_dir,
                            symlinks=True,
                        )

        # generate single framework for group
        lipo_archs_args = []

        for item in target_data:
            if item["group"] == group:
                arch_dir = os.path.join(
                    c.proj_path, "build", build_dir_prefix, item["group"], item["arch"]
                )

                if f.dir_exists(
                    os.path.join(arch_dir, "lib", "xplpc.framework", "Versions")
                ):
                    lipo_archs_args.append(
                        os.path.join(
                            arch_dir, "lib", "xplpc.framework", "Versions", "A", "xplpc"
                        )
                    )
                else:
                    lipo_archs_args.append(
                        os.path.join(arch_dir, "lib", "xplpc.framework", "xplpc")
                    )

        lipo_args = [
            "lipo",
            "-create",
            "-output",
        ]

        if f.dir_exists(os.path.join(group_framework_dir, "Versions")):
            lipo_args.extend(
                [os.path.join(group_framework_dir, "Versions", "A", "xplpc")]
            )
        else:
            lipo_args.extend([os.path.join(group_framework_dir, "xplpc")])

        lipo_args.extend(lipo_archs_args)
        r.run(lipo_args, cwd=c.proj_path)

        # generate single swift module header
        swift_module_headers_dir = os.path.join(group_framework_dir, "Headers")
        swift_module_header_file = os.path.join(
            swift_module_headers_dir, "xplpc-Swift.h"
        )
        has_swift_module_header_file = f.file_exists(swift_module_header_file)

        if has_swift_module_header_file:
            swift_module_header_content = ""

            for item in target_data:
                if item["group"] == group:
                    arch_dir = os.path.join(
                        c.proj_path,
                        "build",
                        build_dir_prefix,
                        item["group"],
                        item["arch"],
                    )
                    framework_dir = os.path.join(arch_dir, "lib", "xplpc.framework")
                    header_file = os.path.join(
                        framework_dir, "Headers", "xplpc-Swift.h"
                    )

                    if f.file_exists(header_file):
                        file_content = f.get_file_contents(header_file)

                        start_content = file_content.find("#if 0")
                        end_content = file_content.find(
                            "#else\n#error unsupported Swift architecture\n#endif"
                        )

                        if start_content > -1 and end_content > -1:
                            extracted_content = file_content[
                                start_content + 5 : end_content
                            ]
                            swift_module_header_content += extracted_content

            if swift_module_header_content:
                swift_module_header_content = f"#if 0\n{swift_module_header_content}#else\n#error unsupported Swift architecture\n#endif"

                f.set_file_content(
                    swift_module_header_file, swift_module_header_content
                )

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
def get_target_data_for_platform(platform):
    if platform == "ios":
        return c.targets["swift-ios"]
    elif platform == "macos":
        return c.targets["swift-macos"]
    elif platform == "ios-flutter":
        return c.targets["swift-ios-flutter"]
    elif platform == "macos-flutter":
        return c.targets["swift-macos-flutter"]
    elif platform == "test":
        return c.targets["swift-test"]

    if platform:
        l.e(f"Invalid platform: {platform}")
    else:
        l.e("Define a valid platform")
