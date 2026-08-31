import hashlib
import json
import os
import re
import subprocess

from pygemstones.io import file as f
from pygemstones.util import log as l

from core import conan, run, tool, util
from core import config as c


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

    incremental = util.get_param_incremental()
    l.i(f"Incremental: {incremental}")

    interface = util.get_param_interface()
    l.i(f"Interface: {interface}")

    platform = util.get_param_platform()
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

    incremental = util.get_param_incremental()
    l.i(f"Incremental: {incremental}")

    platform = util.get_param_platform()
    l.i(f"Platform: {platform}")
    target_data = get_target_data_for_platform(platform)

    # build
    l.i("Building...")

    do_build_xcframework(
        target=target,
        platform=platform,
        target_data=target_data,
    )

    link_xcframework_into_flutter_plugin(platform)

    l.ok()


# -----------------------------------------------------------------------------
def link_xcframework_into_flutter_plugin(platform):
    # The podspec vendors the framework from inside the plugin, so the pointer to what was just
    # built is build output rather than something a checkout carries.

    apple_platform = {"ios-flutter": "ios", "macos-flutter": "macos"}.get(platform)

    if not apple_platform:
        return

    link = os.path.join(
        c.proj_path, "flutter", "plugin", apple_platform, "xplpc.xcframework"
    )
    target = os.path.join(
        "..", "..", "..", "build", f"swift-{platform}-xcframework", "xplpc.xcframework"
    )

    if os.path.islink(link) or os.path.exists(link):
        os.remove(link)

    os.symlink(target, link)
    l.i(f"Linked: {link}")


# -----------------------------------------------------------------------------
def run_task_test():
    # the simulator runs the same suite through the sample, since an XCTest bundle needs a host application there
    if c.options["--platform"] == "ios":
        run_task_test_ios()
        return

    if c.options["--platform"] == "ios-device":
        run_task_test_ios_device()
        return

    # check
    tool.check_tool_cmake()

    # the suite is an xctest bundle the runner opens with dlopen, and only the sanitizer that needs no early interceptors survives that
    sanitizer = util.get_param_sanitizer()

    if sanitizer in ["address", "thread"]:
        l.e(
            f"The {sanitizer} sanitizer cannot reach this suite, its runtime has to be in the process before it starts"
        )

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

    run.run(
        ["ctest", "-C", build_type, "--output-on-failure"],
        cwd=os.path.join(build_dir, group, arch),
    )

    util.show_file_contents(
        os.path.join(build_dir, group, arch, "Testing", "Temporary", "LastTest.log")
    )

    l.ok()


# -----------------------------------------------------------------------------
# the runner decides which simulators exist, so the first available iphone is used rather than a fixed model
def get_simulator_id():
    output = subprocess.check_output(
        ["xcrun", "simctl", "list", "devices", "available", "--json"],
        text=True,
    )

    for runtime, devices in json.loads(output)["devices"].items():
        if "iOS" not in runtime:
            continue

        for device in devices:
            if device.get("isAvailable") and "iPhone" in device["name"]:
                return device["udid"]

    l.e("No iOS simulator is available")


# -----------------------------------------------------------------------------
def get_connected_device_id():
    output = subprocess.check_output(
        ["xcrun", "xctrace", "list", "devices"],
        text=True,
    )

    for line in output.splitlines():
        if line.startswith("== Devices Offline") or line.startswith("== Simulators"):
            break

        found = re.match(
            r"^(.+) \((\d+\.[\d.]+)\) \(([0-9A-Fa-f-]{25,})\)$", line.strip()
        )

        if found:
            return found.group(3)

    l.e("No iOS device is connected")


# -----------------------------------------------------------------------------
def run_task_test_ios_device():
    # check
    tool.check_tool_xcodebuild()

    # the project is generated, so it is regenerated before testing rather than trusted to be current
    run_task_generate_sample_project()

    # test
    l.i("Testing on the device...")

    sample_dir = os.path.join(c.proj_path, "swift", "sample")

    run.run(
        [
            "xcodebuild",
            "test",
            "-project",
            "Runner.xcodeproj",
            "-scheme",
            "Runner",
            "-configuration",
            "Debug",
            "-destination",
            f"platform=iOS,id={get_connected_device_id()}",
            "-derivedDataPath",
            os.path.join(c.proj_path, "build", "swift-ios-device"),
            "-only-testing:RunnerTests",
        ],
        cwd=sample_dir,
    )

    l.ok()


# -----------------------------------------------------------------------------
def write_simulator_spec():
    # the committed spec names the plain xcframework, and rewriting it in place would leave the repository holding a sanitizer build
    sample_dir = os.path.join(c.proj_path, "swift", "sample")
    spec = os.path.join(c.proj_path, "build", "project-simulator.yml")

    original = f.get_file_contents(os.path.join(sample_dir, "project.yml"))

    f.set_file_content(
        spec,
        original.replace(
            "swift-ios-xcframework",
            "swift-ios-simulator-xcframework",
        ),
    )

    return spec


# -----------------------------------------------------------------------------
def run_task_test_ios():
    # check
    tool.check_tool_xcodebuild()

    # a device triple has no thread sanitizer runtime, so an instrumented run links the simulator only xcframework
    sanitizer = util.get_param_sanitizer()

    # the project is generated, so it is regenerated before testing rather than trusted to be current
    run_task_generate_sample_project(
        write_simulator_spec() if sanitizer != "none" else None
    )

    # test
    l.i("Testing on the simulator...")

    sample_dir = os.path.join(c.proj_path, "swift", "sample")

    run.run(
        [
            "xcodebuild",
            "test",
            "-project",
            "Runner.xcodeproj",
            "-scheme",
            "Runner",
            "-configuration",
            "Debug",
            "-destination",
            f"platform=iOS Simulator,id={get_simulator_id()}",
            "-derivedDataPath",
            os.path.join(c.proj_path, "build", "swift-ios-sample"),
            "-only-testing:RunnerTests",
            "-only-testing:RunnerUITests",
            f"-enableThreadSanitizer={'YES' if sanitizer == 'thread' else 'NO'}",
            f"-enableUndefinedBehaviorSanitizer={'YES' if sanitizer == 'undefined' else 'NO'}",
            f"-enableAddressSanitizer={'YES' if sanitizer == 'address' else 'NO'}",
            "CODE_SIGNING_ALLOWED=NO",
        ],
        cwd=sample_dir,
    )

    # the generated project is committed, so it is left naming the xcframework a plain build produces
    if sanitizer != "none":
        run_task_generate_sample_project()

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_sample():
    # check
    tool.check_tool_xcodebuild()

    # the project is generated, so it is regenerated before building rather than trusted to be current
    run_task_generate_sample_project()

    # build
    l.i("Building sample for the simulator...")

    sample_dir = os.path.join(c.proj_path, "swift", "sample")

    run.run(
        [
            "xcodebuild",
            "build",
            "-project",
            "Runner.xcodeproj",
            "-scheme",
            "Runner",
            "-configuration",
            "Debug",
            "-destination",
            "generic/platform=iOS Simulator",
            "-derivedDataPath",
            os.path.join(c.proj_path, "build", "swift-ios-sample"),
            "CODE_SIGNING_ALLOWED=NO",
        ],
        cwd=sample_dir,
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_generate_sample_project(spec=None):
    # check
    tool.check_tool_xcodegen()

    # generate
    l.i("Generating Xcode project for sample...")

    sample_dir = os.path.join(c.proj_path, "swift", "sample")
    generate_command = ["xcodegen", "generate"]

    if spec:
        generate_command += [
            "--spec",
            spec,
            "--project",
            sample_dir,
            "--project-root",
            sample_dir,
        ]

    run.run(generate_command, cwd=sample_dir)

    l.i(
        f"The project was generated here: {os.path.join(sample_dir, 'Runner.xcodeproj')}"
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_generate_spm_package():
    # configure
    l.i("Configuring...")

    url = util.get_param_url() or get_spm_default_url()
    l.i(f"Url: {url}")

    xcframework_dir = os.path.join(
        c.proj_path, "build", "swift-apple-xcframework", "xplpc.xcframework"
    )

    if not f.dir_exists(xcframework_dir):
        l.e(
            "The apple xcframework was not found, build it with: swift-build --platform apple && swift-build-xcframework --platform apple"
        )

    # archive
    l.i("Archiving...")

    archive_file = os.path.join(
        c.proj_path, "build", "swift-apple-xcframework", "xplpc.xcframework.zip"
    )

    f.remove_file(archive_file)

    # ditto keeps the symlinks a macos framework bundle is built from, which zip flattens into copies
    run.run(
        [
            "ditto",
            "-c",
            "-k",
            "--sequesterRsrc",
            "--keepParent",
            xcframework_dir,
            archive_file,
        ],
        cwd=c.proj_path,
    )

    checksum = get_file_checksum(archive_file)
    l.i(f"Checksum: {checksum}")

    # generate
    l.i("Generating package...")

    package_file = os.path.join(
        c.proj_path, "build", "swift-apple-xcframework", "Package.swift"
    )
    f.set_file_content(
        package_file, get_spm_package_content(c.targets["swift-apple"], url, checksum)
    )

    l.i(f"The archive was generated here: {archive_file}")
    l.i(f"The package was generated here: {package_file}")

    l.ok()


# -----------------------------------------------------------------------------
def get_file_checksum(path):
    digest = hashlib.sha256()

    with open(path, "rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)

    return digest.hexdigest()


# -----------------------------------------------------------------------------
def get_spm_default_url():
    # The version is read where the build declares it, so a bump is not something anyone has to remember here.
    content = f.get_file_contents(os.path.join(c.proj_path, "CMakeLists.txt"))
    found = re.search(r'set\(XPLPC_VERSION "([^"]+)"', content)

    if not found:
        l.e("The project version was not found in CMakeLists.txt")

    version = found.group(1)

    return f"https://github.com/xplpc/xplpc/releases/download/v{version}/xplpc.xcframework.zip"


# -----------------------------------------------------------------------------
def get_spm_package_content(target_data, url, checksum):
    template_file = os.path.join(c.proj_path, "swift", "spm", "Package.swift.in")

    if not f.file_exists(template_file):
        l.e(f"The swift package template was not found: {template_file}")

    platforms = get_spm_platforms(target_data)
    platform_lines = "\n".join([f"        .{item}," for item in platforms])

    content = f.get_file_contents(template_file)

    return (
        content.replace("@PLATFORMS@", platform_lines)
        .replace("@URL@", url)
        .replace("@CHECKSUM@", checksum)
    )


# -----------------------------------------------------------------------------
def get_spm_platforms(target_data):
    group_platforms = {
        "ios": "iOS",
        "ios-simulator": "iOS",
        "macos": "macOS",
        "tvos": "tvOS",
        "tvos-simulator": "tvOS",
        "watchos": "watchOS",
        "watchos-simulator": "watchOS",
        "mac-catalyst": "macCatalyst",
    }

    # the package has to declare what the slices were built for, so a bump in a target is not something anyone has to remember here
    minimums = {}

    for item in target_data:
        platform = group_platforms.get(item["group"])

        if not platform:
            l.e(f"Group without a swift package platform: {item['group']}")

        version = tuple(int(part) for part in item["deployment_target"].split("."))

        if platform not in minimums or version < minimums[platform]:
            minimums[platform] = version

    order = ["iOS", "macOS", "tvOS", "watchOS", "macCatalyst"]

    return [
        f"{platform}({get_spm_platform_version(minimums[platform])})"
        for platform in order
        if platform in minimums
    ]


# -----------------------------------------------------------------------------
def get_spm_platform_version(version):
    # swift package manager names a major version alone, except for the macos releases that never left ten
    if version[0] == 10:
        return f".v10_{version[1]}"

    # a floor carrying a minor has no case to name it, and rounding it down would promise a version the slices were not built for
    if len(version) > 1 and version[1] != 0:
        return '"' + ".".join(str(part) for part in version) + '"'

    return f".v{version[0]}"


# -----------------------------------------------------------------------------
def run_task_lint():
    tool.check_tool_swift_linter()

    l.i("Linting Swift files...")

    run.run(["swiftlint", "lint", "--strict"], cwd=c.proj_path)

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
            formatter=lambda file_item: run.run(
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

    incremental = util.get_param_incremental()
    if not incremental:
        f.recreate_dir(build_dir)

    # dependencies
    no_deps = util.get_param_no_deps()

    if not incremental and not no_deps and c.dependency_tool == "conan":
        for item in target_data:
            l.i(f"Building dependencies for arch {item['arch']}/{item['group']}...")

            arch_dir = os.path.join(conan_build_dir, item["group"], item["arch"])
            f.recreate_dir(arch_dir)

            conan.install(item, build_type, arch_dir)

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
            f"-DXPLPC_SANITIZER={util.get_param_sanitizer()}",
            "-DXPLPC_ADD_CUSTOM_DATA=ON",
            f"-DXPLPC_DEPENDENCY_TOOL={c.dependency_tool}",
            f"-DPLATFORM={item['platform']}",
            f"-DDEPLOYMENT_TARGET={item['deployment_target']}",
            f"-DCMAKE_OSX_DEPLOYMENT_TARGET={item['deployment_target']}",
            f"-DSDK_VERSION={item['sdk_version']}",
            f"-DARCHS={item['arch']}",
        ]

        # a slice built through a destination needs a scheme to aim it at, and its own deployment target, which the toolchain leaves to the sdk
        if "xcode_destination" in item:
            run_args.append("-DCMAKE_XCODE_GENERATE_SCHEME=ON")
            run_args.append(
                f"-DCMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET={item['deployment_target']}"
            )

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

        run.run(run_args)

        # build
        if "xcode_destination" in item:
            # xcodebuild only honours a destination when it is given a scheme, and cmake builds a target, which is why a variant like mac catalyst is silently built as its base platform
            run.run(
                [
                    "xcodebuild",
                    "-project",
                    os.path.join(arch_dir, "xplpc.xcodeproj"),
                    "-scheme",
                    "xplpc",
                    "-configuration",
                    build_type,
                    "-destination",
                    item["xcode_destination"],
                    "build",
                ]
            )
        else:
            run.run(["cmake", "--build", arch_dir, "--config", build_type])

        # a slice built for the wrong platform still links and still passes, so the only way to know is to read it back
        if "expected_platform" in item:
            check_built_platform(arch_dir, item["expected_platform"])


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

    incremental = util.get_param_incremental()
    if not incremental:
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

                lipo_archs_args.append(
                    get_framework_binary(
                        os.path.join(arch_dir, "lib", "xplpc.framework")
                    )
                )

        lipo_args = [
            "lipo",
            "-create",
            "-output",
            get_framework_binary(group_framework_dir),
        ]

        lipo_args.extend(lipo_archs_args)
        run.run(lipo_args, cwd=c.proj_path)

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

    run.run(xcodebuild_command, cwd=c.proj_path, silent=True)

    l.i(f"The xcframework was generated here: {xcframework_dir}")


# -----------------------------------------------------------------------------
def get_framework_binary(framework_dir):
    if f.dir_exists(os.path.join(framework_dir, "Versions")):
        return os.path.join(framework_dir, "Versions", "A", "xplpc")

    return os.path.join(framework_dir, "xplpc")


# -----------------------------------------------------------------------------
def check_built_platform(arch_dir, expected_platform):
    binary = get_framework_binary(os.path.join(arch_dir, "lib", "xplpc.framework"))

    output = subprocess.run(
        ["vtool", "-show-build", binary],
        capture_output=True,
        text=True,
        check=True,
    ).stdout

    built_platforms = re.findall(r"^\s*platform (\S+)", output, re.MULTILINE)

    if not built_platforms:
        l.e(f"No platform was reported for: {binary}")

    for built_platform in built_platforms:
        if built_platform != expected_platform:
            l.e(
                f"Built for {built_platform} where {expected_platform} was asked for: {binary}"
            )


# -----------------------------------------------------------------------------
def get_target_data_for_platform(platform):
    if platform == "apple":
        return c.targets["swift-apple"]
    elif platform == "ios":
        return c.targets["swift-ios"]
    elif platform == "macos":
        return c.targets["swift-macos"]
    elif platform == "ios-flutter":
        return c.targets["swift-ios-flutter"]
    elif platform == "macos-flutter":
        return c.targets["swift-macos-flutter"]
    elif platform == "test":
        return c.targets["swift-test"]
    elif platform == "ios-simulator":
        # the thread sanitizer has no runtime for a device triple, so the slices it can instrument are taken on their own
        return [
            item for item in c.targets["swift-ios"] if item["group"] == "ios-simulator"
        ]

    if platform:
        l.e(f"Invalid platform: {platform}")
    else:
        l.e("Define a valid platform")
