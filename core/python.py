import os

from pygemstones.io import file as f
from pygemstones.util import log as l

from core import config as c
from core import host, run, tool, util


# -----------------------------------------------------------------------------
def run_task_build():
    tool.check_tool_python()

    l.i("Copying lib files...")
    build_dir = os.path.join("build", "python")
    f.recreate_dir(build_dir)

    module_dir = os.path.join("python", "lib")
    f.copy_all(module_dir, build_dir)

    l.i("Copying binary files...")
    lib_arch = host.get_arch_path()
    binary_dir = os.path.join("build", "c-shared", lib_arch, host.get_lib_binary_dir())
    build_binary_dir = os.path.join(build_dir, "src", "xplpc", "lib", lib_arch)
    f.copy_all(binary_dir, build_binary_dir)

    # build
    l.i("Building...")

    # A source distribution cannot produce the native library, so publishing one would let pip
    # install a package on a platform the wheel correctly refuses and fail on the first call.
    run.run(["python3", "-m", "build", "--wheel"], cwd=build_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_install():
    tool.check_tool_pip()

    if not util.get_param_wheel():
        l.i("Installing development package...")

        lib_dir = os.path.join("python", "lib")

        run.run(
            ["python3", "-m", "pip", "install", "-e", ".", "--force-reinstall"],
            cwd=lib_dir,
        )

        l.ok()
        return

    l.i("Searching for package...")

    dist_dir = os.path.join("build", "python", "dist")
    packages = f.find_files(dist_dir, "*.whl")

    if len(packages) == 0:
        l.e("No package found, you need build it first")

    package = packages[0]
    l.i(f"Package found: {package}")

    l.i("Installing wheel package...")
    run.run(["python3", "-m", "pip", "install", package, "--force-reinstall"])

    l.ok()


# -----------------------------------------------------------------------------
def run_task_test():
    tool.check_tool_pytest()

    if util.get_param_sanitizer() != "none":
        l.e(
            "The sanitizer instruments the native library, so build it with c-build-shared --sanitizer and the suite picks it up from the environment"
        )

    l.i("Testing...")
    python_dir = os.path.join("python", "tests")
    run.run(["pytest"], cwd=python_dir)
    l.ok()


# -----------------------------------------------------------------------------
def run_task_run_sample():
    tool.check_tool_python()

    l.i("Running...")
    sample_dir = os.path.join("python", "sample", "src")
    run.run(["python3", "main.py"], cwd=sample_dir)
    l.ok()


# -----------------------------------------------------------------------------
def run_task_pyinstaller():
    # The task runs pyinstaller through poetry, so poetry is the only tool that has to be on the path.
    tool.check_tool_poetry()

    l.i("Running...")

    dist_dir = os.path.join(c.proj_path, "build", "pyinstaller")
    temp_dir = os.path.join(c.proj_path, "build", "pyinstaller-temp")

    f.recreate_dir(dist_dir)
    f.recreate_dir(temp_dir)

    sample_dir = os.path.join("python", "sample", "pyinstaller")

    # Poetry adopts an activated environment when it finds one, and syncing removes everything the lock does not name, so it would empty the environment this task is running in.
    sample_env = os.environ.copy()
    sample_env.pop("VIRTUAL_ENV", None)

    # Analysing the sample imports kivymd, which asks kivy for a font size and makes it open a window, so packaging would need a display it never uses.
    sample_env["SDL_VIDEODRIVER"] = "dummy"
    sample_env["KIVY_DPI"] = "96"
    sample_env["KIVY_METRICS_DENSITY"] = "1"

    run.run(["poetry", "sync"], cwd=sample_dir, env=sample_env)

    run.run(
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
        env=sample_env,
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_lint():
    # check
    tool.check_tool_python_linter()

    # lint
    l.i("Linting Python files...")

    run.run(
        [
            "ruff",
            "check",
            "--config",
            os.path.join(c.proj_path, "ruff.toml"),
            os.path.join(c.proj_path, "xplpc.py"),
            os.path.join(c.proj_path, "conanfile.py"),
            os.path.join(c.proj_path, "core"),
            os.path.join(c.proj_path, "python", "lib", "src"),
            os.path.join(c.proj_path, "python", "tests"),
        ],
        cwd=c.proj_path,
    )

    # The package is annotated, so the annotations are checked rather than taken on trust.
    l.i("Checking Python types...")

    run.run(
        [
            "mypy",
            "--config-file",
            os.path.join(c.proj_path, "mypy.ini"),
            os.path.join(c.proj_path, "python", "lib", "src", "xplpc"),
        ],
        cwd=c.proj_path,
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
            formatter=lambda file_item: run.run(
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
