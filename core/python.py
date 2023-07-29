import os
import platform

from pygemstones.io import file as f
from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import config as c
from core import tool, util


# -----------------------------------------------------------------------------
def run_task_build():
    tool.check_tool_python()

    l.i("Copying lib files...")
    build_dir = os.path.join("build", "python")
    f.recreate_dir(build_dir)

    module_dir = os.path.join("python", "lib")
    f.copy_all(module_dir, build_dir)

    l.i("Copying binary files...")
    lib_arch = get_binary_arch_path()
    binary_dir = os.path.join("build", "c-shared", lib_arch, util.get_lib_binary_dir())
    build_binary_dir = os.path.join(build_dir, "src", "xplpc", "lib", lib_arch)
    f.copy_all(binary_dir, build_binary_dir)

    # build
    l.i("Building...")
    r.run(["python3", "setup.py", "sdist", "bdist_wheel"], cwd=build_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_install():
    tool.check_tool_pip()

    use_dev = True

    if use_dev:
        # install
        l.i("Installing development package...")

        lib_dir = os.path.join("python", "lib")

        r.run(
            ["python3", "-m", "pip", "install", "-e", ".", "--force-reinstall"],
            cwd=lib_dir,
        )
    else:
        # find package
        l.i("Searching for package...")
        dist_dir = os.path.join("build", "python", "dist")
        packages = f.find_files(dist_dir, "*.whl")

        if len(packages) > 0:
            package = packages[0]
            l.i(f"Package found: {package}")
        else:
            l.e("No package found, you need build it first")

        # install
        l.i("Installing wheel package...")
        r.run(["python3", "-m", "pip", "install", package, "--force-reinstall"])

    l.ok()


# -----------------------------------------------------------------------------
def run_task_test():
    tool.check_tool_pytest()

    l.i("Testing...")
    python_dir = os.path.join("python", "tests")
    r.run(["pytest"], cwd=python_dir)
    l.ok()


# -----------------------------------------------------------------------------
def run_task_sample():
    tool.check_tool_python()

    l.i("Running...")
    sample_dir = os.path.join("python", "sample", "src")
    r.run(["python3", "main.py"], cwd=sample_dir)
    l.ok()


# -----------------------------------------------------------------------------
def run_task_pyinstaller():
    tool.check_tool_pyinstaller()

    l.i("Running...")

    dist_dir = os.path.join(c.proj_path, "build", "pyinstaller")
    temp_dir = os.path.join(c.proj_path, "build", "pyinstaller-temp")

    f.recreate_dir(dist_dir)
    f.recreate_dir(temp_dir)

    sample_dir = os.path.join("python", "sample", "pyinstaller")

    r.run(["poetry", "install", "--sync"], cwd=sample_dir)

    r.run(
        [
            "poetry",
            "run",
            "pyinstaller",
            "pyinstaller.spec",
            "--distpath",
            dist_dir,
            "--workpath",
            temp_dir,
            "--noconfirm",
            "--clean",
        ],
        cwd=sample_dir,
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_format():
    # check
    tool.check_tool_python_formatter()

    # format
    path_list = [
        {
            "path": os.path.join(c.proj_path, "xplpc.py"),
        },
        {
            "path": os.path.join(c.proj_path, "core"),
            "patterns": ["*.py"],
        },
        {
            "path": os.path.join(c.proj_path, "conanfile.py"),
        },
        {
            "path": os.path.join(c.proj_path, "conan"),
            "patterns": ["*.py"],
        },
        {
            "path": os.path.join(c.proj_path, "python"),
            "patterns": ["*.py"],
        },
    ]

    if path_list:
        l.i("Formatting Python files...")

        util.run_format(
            path_list=path_list,
            formatter=lambda file_item: r.run(
                [
                    "black",
                    "-q",
                    file_item,
                ],
                cwd=c.proj_path,
            ),
            ignore_path_list=[],
        )

        l.ok()
    else:
        l.i("No Python files found to format")


# -----------------------------------------------------------------------------
def get_binary_arch_path():
    arch = platform.machine().lower()
    lib_arch = ""

    if arch == "armv7l" or arch == "armv7":
        lib_arch = "arm32"
    elif arch == "aarch64" or arch == "arm64":
        lib_arch = "arm64"
    elif arch == "i686" or arch == "x86":
        lib_arch = "x86"
    elif arch == "x86_64" or arch == "amd64":
        lib_arch = "x86_64"
    else:
        l.e(f"The architecture {arch} is not supported.")

    return lib_arch
