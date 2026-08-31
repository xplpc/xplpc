import os

from pygemstones.io import file as f
from pygemstones.system import env as e
from pygemstones.system import platform as p
from pygemstones.util import log as l

from core import config as cfg
from core import run


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
    return os.path.join(directory, f"{name}.exe" if p.is_windows() else name)


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
def run_format(path_list, formatter, ignore_path_list):
    # pylint: disable=too-many-nested-blocks
    for path_list_item in path_list:
        patterns = path_list_item["patterns"] if "patterns" in path_list_item else None

        if patterns:
            for pattern_item in patterns:
                files = f.find_files(
                    path_list_item["path"],
                    pattern_item,
                    recursive=True,
                    follow_links=True,
                )

                for file_item in files:
                    ignore_file = False

                    if ignore_path_list:
                        for ignore_path in ignore_path_list:
                            if ignore_path in os.path.abspath(file_item):
                                ignore_file = True

                    if not ignore_file:
                        if f.file_exists(os.path.abspath(file_item)):
                            rel_path = os.path.relpath(file_item)

                            l.i(f"Formatting file: {rel_path}...")

                            formatter(file_item)
        else:
            file_item = path_list_item["path"] if "path" in path_list_item else None

            if file_item:
                ignore_file = False

                if ignore_path_list:
                    for ignore_path in ignore_path_list:
                        if ignore_path in os.path.abspath(file_item):
                            ignore_file = True

                if not ignore_file:
                    if f.file_exists(os.path.abspath(file_item)):
                        rel_path = os.path.relpath(file_item)

                        l.i(f"Formatting file: {rel_path}...")

                        formatter(file_item)
    # pylint: enable=too-many-nested-blocks


# -----------------------------------------------------------------------------
def get_param_build_type(target, platform=None, format=None):
    param_build_type = cfg.options["--build"]

    if not param_build_type:
        if target == "kotlin":
            if platform and platform in ["android", "flutter"]:
                param_build_type = cfg.build_type_kotlin
            else:
                param_build_type = cfg.build_type
        else:
            param_build_type = cfg.build_type

    if format:
        if format == "cmake":
            param_build_type = get_cmake_build_type(param_build_type)

    return param_build_type


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
def get_param_incremental():
    return cfg.options["--incremental"]


# -----------------------------------------------------------------------------
def get_param_no_deps():
    return cfg.options["--no-deps"]


# -----------------------------------------------------------------------------
def get_cmake_build_type(build_type):
    build_type = build_type.lower()

    if build_type == "debug":
        return "Debug"
    elif build_type == "release":
        return "Release"
    elif build_type == "relwithdebinfo":
        return "RelWithDebInfo"
    elif build_type == "profile":
        return "Profile"

    l.e(f"Invalid build type: {build_type}")
