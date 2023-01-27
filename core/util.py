import os

from pygemstones.io import file as f
from pygemstones.system import env as e
from pygemstones.system import platform as p
from pygemstones.system import runner as r
from pygemstones.util import log as l


# -----------------------------------------------------------------------------
def exec_name(name):
    if p.is_windows():
        return f"{name}.exe"

    return name


# -----------------------------------------------------------------------------
def run_name(name):
    if p.is_windows():
        return f"{name}.exe"

    return f"./{name}"


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

    r.run(run_args, shell=shell)


# -----------------------------------------------------------------------------
def run_gradle(commands, cwd):
    shell = False

    if p.is_windows():
        run_args = ["gradlew.bat"]
        shell = True
    else:
        run_args = ["./gradlew"]

    run_args.extend(commands)

    r.run(run_args, shell=shell, cwd=cwd)


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
