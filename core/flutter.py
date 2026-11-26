import os

from pygemstones.io import file as f
from pygemstones.system import platform as p
from pygemstones.util import log as l

from core import config as c
from core import run, tool, util


# -----------------------------------------------------------------------------
def run_task_format():
    # check
    tool.check_tool_flutter()

    # format
    l.i("Formatting Dart files...")

    project_dir = os.path.join(c.proj_path, "flutter", "plugin")

    run.run(["dart", "format", "lib", "test"], cwd=project_dir)
    run.run(["dart", "format", "lib"], cwd=os.path.join(project_dir, "example"))

    l.ok()


# -----------------------------------------------------------------------------
def get_library_path():
    # A suite runs on the host vm, where nothing bundled the native library, so the runner points at what the build for this platform wrote.

    if p.is_macos():
        path = os.path.join(
            c.proj_path,
            "build",
            "swift-macos-flutter-xcframework",
            "xplpc.xcframework",
            "macos-arm64_x86_64",
            "xplpc.framework",
            "Versions",
            "A",
            "xplpc",
        )

        if os.path.exists(path):
            return path

        l.e(
            "The native library was not found, build it with: python3 xplpc.py swift-build --interface --platform macos-flutter && python3 xplpc.py swift-build-xcframework --platform macos-flutter"
        )

    return util.get_shared_library_path()


# -----------------------------------------------------------------------------
def run_task_test():
    # check
    tool.check_tool_flutter()

    if util.get_param_sanitizer() != "none":
        l.e(
            "The sanitizer instruments the native library, so build it with c-build-shared --sanitizer and the suite picks it up from the environment"
        )

    # build
    l.i("Testing...")
    project_dir = os.path.join(c.proj_path, "flutter", "plugin")

    # Flutter copies the plugin into its own build directory without leaving that directory out, so every run would copy the previous copies again.
    for platform in ["ios", "macos"]:
        f.remove_dir(os.path.join(project_dir, "build", platform, "SourcePackages"))

    # dependencies
    run.run(["flutter", "pub", "get"], cwd=project_dir)

    # build runner
    run.run(
        [
            "dart",
            "run",
            "build_runner",
            "build",
        ],
        cwd=project_dir,
    )

    # analyze
    run.run(["dart", "analyze", "lib", "test"], cwd=project_dir)

    # The example is the only consumer of the public surface, so a broken entry point shows up here.
    example_dir = os.path.join(project_dir, "example")
    run.run(["flutter", "pub", "get"], cwd=example_dir)
    run.run(["dart", "run", "build_runner", "build"], cwd=example_dir)
    run.run(["dart", "analyze", "lib"], cwd=example_dir)

    # test
    env = os.environ.copy()
    env["XPLPC_LIBRARY_PATH"] = get_library_path()

    run.run(["flutter", "test"], cwd=project_dir, env=env)

    l.ok()
