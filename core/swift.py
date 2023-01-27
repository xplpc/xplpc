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
    build_dir = os.path.join(c.proj_path, "build", "swift")
    f.recreate_dir(build_dir)

    for item in c.swift_framework_list:
        l.i(f"Building for arch {item['arch']}...")

        arch_dir = os.path.join(build_dir, item["arch"])

        toolchain_file = os.path.join(c.proj_path, "cmake", "ios.toolchain.cmake")

        r.run(
            [
                "cmake",
                "-S",
                ".",
                "-B",
                arch_dir,
                "-GXcode",
                f"-DCMAKE_BUILD_TYPE={c.build_type}",
                f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}",
                "-DXPLPC_TARGET=swift",
                "-DXPLPC_ADD_CUSTOM_DATA=ON",
                f"-DPLATFORM={item['platform']}",
                f"-DDEPLOYMENT_TARGET={item['version']}",
            ]
        )

        r.run(["cmake", "--build", arch_dir, "--config", c.build_type])

    l.ok()


# -----------------------------------------------------------------------------
def run_task_test():
    # check
    tool.check_tool_cmake()

    # environment
    os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # build
    l.i("Testing...")
    build_dir = os.path.join(c.proj_path, "build", "swift-test")
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
                f"-DCMAKE_BUILD_TYPE={c.build_type}",
                "-DXPLPC_TARGET=swift",
                "-DXPLPC_ADD_CUSTOM_DATA=ON",
                "-DXPLPC_ENABLE_TESTS=ON",
            ]
        )

        r.run(["cmake", "--build", arch_dir, "--config", c.build_type])

        r.run(["ctest", "-C", c.build_type, "--output-on-failure"], cwd=arch_dir)

        util.show_file_contents(
            os.path.join(arch_dir, "Testing", "Temporary", "LastTest.log")
        )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_xcframework():
    # check
    tool.check_tool_xcodebuild()

    # merge groups
    l.i("Building...")

    groups = []

    for item in c.swift_framework_list:
        if "group" in item:
            if item["group"] not in groups:
                groups.append(item["group"])

    # generate framework for each group
    groups_command = []
    build_dir = os.path.join(c.proj_path, "build", "swift-group")
    f.recreate_dir(build_dir)

    for group in groups:
        l.i(f"Building for group {group}...")

        # get first framework data for current group
        base_framework_arch = None
        arch_dir = None

        for item in c.swift_framework_list:
            if item["group"] == group:
                base_framework_arch = item["arch"]
                arch_dir = os.path.join(c.proj_path, "build", "swift", item["arch"])

        if not base_framework_arch:
            l.e(f"Group framework was not found: {group}")

        # copy base framework
        framework_dir = os.path.join(arch_dir, "lib", c.build_type, "xplpc.framework")
        group_xcframework_dir = os.path.join(build_dir, group, "xplpc.framework")

        f.copy_all(
            framework_dir,
            group_xcframework_dir,
        )

        # generate single framework for group
        lipo_archs_args = []

        for item in c.swift_framework_list:
            if item["group"] == group:
                arch_dir = os.path.join(c.proj_path, "build", "swift", item["arch"])

                lipo_archs_args.append(
                    os.path.join(
                        arch_dir, "lib", c.build_type, "xplpc.framework", "xplpc"
                    )
                )

        lipo_args = [
            "lipo",
            "-create",
            "-output",
        ]

        if f.dir_exists(
            os.path.join(
                group_xcframework_dir,
                "Versions",
            )
        ):
            lipo_args.extend(
                [
                    os.path.join(group_xcframework_dir, "Versions", "A", "xplpc"),
                ]
            )
        else:
            lipo_args.extend(
                [
                    os.path.join(group_xcframework_dir, "xplpc"),
                ]
            )

        lipo_args.extend(lipo_archs_args)
        r.run(lipo_args, cwd=c.proj_path)

        # add final framework to group
        groups_command.append("-framework")
        groups_command.append(group_xcframework_dir)

    # generate xcframework
    xcframework_dir = os.path.join(
        c.proj_path, "build", "swift-xcframework", "xplpc.xcframework"
    )

    f.remove_dir(xcframework_dir)

    xcodebuild_command = ["xcodebuild", "-create-xcframework"]
    xcodebuild_command += groups_command
    xcodebuild_command += ["-output", xcframework_dir]

    r.run(xcodebuild_command, cwd=c.proj_path, silent=True)

    l.i(f"The xcframework was generated here: {xcframework_dir}")

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
