import os

from pygemstones.io import file as f
from pygemstones.system import env as e
from pygemstones.system import platform as p
from pygemstones.util import log as l

from core import config as cfg
from core import host, run


# -----------------------------------------------------------------------------
def get_target_data_for_host():
    if p.is_macos():
        return cfg.targets["platform-macos"]

    if p.is_windows():
        return cfg.targets["platform-windows"]

    if p.is_linux():
        return cfg.targets["platform-linux"]

    l.e("Unknown platform")


# -----------------------------------------------------------------------------
def exec_name(name):
    if p.is_windows():
        return f"{name}.exe"

    return name


# -----------------------------------------------------------------------------
def run_name(directory, name):
    # Windows resolves the executable against the directory the caller is in rather than the one the child is given, so the path is always complete.
    return os.path.join(directory, exec_name(name))


# -----------------------------------------------------------------------------
def remove_sdkroot_from_env():
    e.remove_var("SDKROOT")


# -----------------------------------------------------------------------------
def show_file_contents(file_path):
    shell = False

    if p.is_windows():
        run_args = ["type"]
        shell = True
    else:
        run_args = ["cat"]

    run_args.append(file_path)

    run.run(run_args, shell=shell)


# -----------------------------------------------------------------------------
def run_gradle(commands, cwd):
    shell = False

    if p.is_windows():
        run_args = ["gradlew.bat"]
        shell = True
    else:
        run_args = ["./gradlew"]

    run_args.extend(commands)

    run.run(run_args, shell=shell, cwd=cwd)


# -----------------------------------------------------------------------------
def run_format(path_list, formatter):
    for path_list_item in path_list:
        path = path_list_item["path"]
        patterns = path_list_item.get("patterns")

        files = [path]

        if patterns:
            files = f.find_files(path, patterns, recursive=True, follow_links=True)

        for file_item in files:
            l.i(f"Formatting file: {os.path.relpath(file_item)}...")

            formatter(file_item)


# -----------------------------------------------------------------------------
def get_param_build_type(target, platform=None):
    param_build_type = cfg.options["--build"]

    if not param_build_type:
        param_build_type = cfg.build_type

        if target == "kotlin" and platform in ["android", "flutter"]:
            param_build_type = cfg.build_type_kotlin

    return get_cmake_build_type(param_build_type)


# -----------------------------------------------------------------------------
def get_param_interface():
    return cfg.options["--interface"]


# -----------------------------------------------------------------------------
def get_param_wheel():
    return cfg.options["--wheel"]


# -----------------------------------------------------------------------------
def get_param_platform():
    param_platform = cfg.options["--platform"]

    if not param_platform:
        l.e("Define a valid platform")

    return param_platform.lower()


# -----------------------------------------------------------------------------
def get_param_url():
    return cfg.options["--url"]


# -----------------------------------------------------------------------------
def get_param_dependency_tool():
    param_tool = cfg.options["--dependency-tool"].lower()

    if param_tool not in ["cpm", "conan"]:
        l.e("Dependency tool need be one of: cpm, conan")

    return param_tool


# -----------------------------------------------------------------------------
def get_param_sanitizer():
    return cfg.options["--sanitizer"] or "none"


# -----------------------------------------------------------------------------
def get_custom_data_args():
    # The tasks build this repository's own mappings into every artifact unless a consumer names a directory of their own.

    path = cfg.options["--custom-data-path"]
    args = ["-DXPLPC_ADD_CUSTOM_DATA=ON"]

    if not path:
        return args

    path = os.path.abspath(path)

    if not os.path.isfile(os.path.join(path, "CMakeLists.txt")):
        l.e(f"The custom data directory carries no CMakeLists.txt: {path}")

    args.append(f"-DXPLPC_CUSTOM_DATA_PATH={path}")

    return args


# -----------------------------------------------------------------------------
def get_param_shuffle():
    return cfg.options["--shuffle"]


# -----------------------------------------------------------------------------
def run_gtest_suite(arch_dir, binary_dir, build_type):
    # Ctest runs each test in a process of its own, so only the binary run whole can show one test leaving state behind for the next.
    if get_param_shuffle():
        run.run(
            [
                run_name(binary_dir, "xplpc-tests"),
                "--gtest_shuffle",
                "--gtest_repeat=2",
            ],
            cwd=binary_dir,
        )

        return

    run.run(["ctest", "-C", build_type, "--output-on-failure"], cwd=arch_dir)

    show_file_contents(os.path.join(arch_dir, "Testing", "Temporary", "LastTest.log"))


# -----------------------------------------------------------------------------
def get_param_incremental():
    return cfg.options["--incremental"]


# -----------------------------------------------------------------------------
def get_param_no_deps():
    return cfg.options["--no-deps"]


# -----------------------------------------------------------------------------
def get_cmake_build_type(build_type):
    cmake_build_types = {
        "debug": "Debug",
        "release": "Release",
        "relwithdebinfo": "RelWithDebInfo",
        "minsizerel": "MinSizeRel",
    }

    if build_type.lower() in cmake_build_types:
        return cmake_build_types[build_type.lower()]

    l.e(f"Invalid build type: {build_type}")


# -----------------------------------------------------------------------------
def get_shared_library_path():
    # A suite runs against what this checkout built, which is named to it rather than found by walking the tree.

    if p.is_windows():
        name = "xplpc.dll"
    elif p.is_macos():
        name = "libxplpc.dylib"
    else:
        name = "libxplpc.so"

    path = os.path.join(
        cfg.proj_path,
        "build",
        "c-shared",
        host.get_arch_path(),
        host.get_lib_binary_dir(),
        name,
    )

    if not os.path.exists(path):
        l.e(
            "The native library was not found, build it with: python3 xplpc.py c-build-shared"
        )

    return path
