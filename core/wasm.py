import os

from pygemstones.io import file as f
from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import conan
from core import config as c
from core import net, tool, util


# -----------------------------------------------------------------------------
def run_task_build():
    # check
    if c.dependency_tool == "cpm":
        tool.check_tool_emsdk()
        emsdk_root = tool.check_and_get_env("EMSDK")
    elif c.dependency_tool == "conan":
        tool.check_tool_conan()

    # environment
    target = "wasm"

    # dependency
    if c.dependency_tool == "cpm":
        os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # configure
    l.i("Configuring...")

    build_type = util.get_param_build_type(target, "cmake")
    l.i(f"Build type: {build_type}")

    dry_run = util.get_param_dry()
    l.i(f"Dry run: {dry_run}")

    target_data = get_target_data_for_platform("wasm")

    build_dir = os.path.join(c.proj_path, "build", target)
    conan_build_dir = os.path.join(c.proj_path, "build", "conan", target)

    # dry run
    if not dry_run:
        f.recreate_dir(build_dir)

    # dependencies
    no_deps = util.get_param_no_deps()

    if not dry_run and not no_deps and c.dependency_tool == "conan":
        for item in target_data:
            l.i(f"Building dependencies for arch {item['arch']}...")

            arch_dir = os.path.join(conan_build_dir, item["arch"])
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
        l.i(f"Building for arch {item['arch']}...")

        arch_dir = os.path.join(build_dir, item["arch"])
        conan_arch_dir = os.path.join(conan_build_dir, item["arch"])

        run_args = [
            "cmake",
            "-S",
            ".",
            "-B",
            arch_dir,
            f"-DXPLPC_TARGET={target}",
            "-DXPLPC_ADD_CUSTOM_DATA=ON",
            f"-DCMAKE_BUILD_TYPE={build_type}",
            f"-DXPLPC_DEPENDENCY_TOOL={c.dependency_tool}",
        ]

        # toolchain
        if c.dependency_tool == "cpm":
            toolchain_file = os.path.join(
                emsdk_root,
                "upstream",
                "emscripten",
                "cmake",
                "Modules",
                "Platform",
                "Emscripten.cmake",
            )
            run_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")
        elif c.dependency_tool == "conan":
            toolchain_file = os.path.join(conan_arch_dir, "conan_toolchain.cmake")
            run_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")

        r.run(run_args)

        # build
        r.run(["cmake", "--build", arch_dir])

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_sample():
    # check
    tool.check_tool_npm()
    tool.check_tool_node()

    # environment
    os.environ["BASE_URL"] = c.wasm_base_url

    # paths
    sample_dir = os.path.join(c.proj_path, "wasm", "sample")

    # dependencies
    dry_run = util.get_param_dry()
    l.i(f"Dry run: {dry_run}")

    if not dry_run:
        l.i("Installing dependencies...")
        r.run(["npm", "install"], cwd=sample_dir)

    # build
    l.i("Building...")
    r.run(["npm", "run", "build"], cwd=sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_run_sample():
    # check
    tool.check_tool_npm()
    tool.check_tool_node()

    # paths
    sample_dir = os.path.join(c.proj_path, "wasm", "sample")

    # dependencies
    dry_run = util.get_param_dry()
    l.i(f"Dry run: {dry_run}")

    if not dry_run:
        l.i("Installing dependencies...")
        r.run(["npm", "install"], cwd=sample_dir)

    # build
    l.i("Building...")
    r.run(["npm", "run", "dev"], cwd=sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_serve_sample():
    dist_dir = os.path.join(c.proj_path, "wasm", "sample", "dist")
    net.serve(dist_dir)
    l.ok()


# -----------------------------------------------------------------------------
def run_task_test():
    # check
    tool.check_tool_npm()

    # paths
    sample_dir = os.path.join(c.proj_path, "wasm", "sample")

    # dependencies
    dry_run = util.get_param_dry()
    l.i(f"Dry run: {dry_run}")

    if not dry_run:
        l.i("Installing dependencies...")
        r.run(["npm", "install"], cwd=sample_dir)

    # test
    l.i("Testing...")
    r.run(["npm", "run", "test:unit"], cwd=sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_format():
    # check
    tool.check_tool_npm()

    # paths
    sample_dir = os.path.join(c.proj_path, "wasm", "sample")

    # format js/css/html
    l.i("Formatting Web files...")
    r.run(["npm", "install"], cwd=sample_dir, silent=True)
    r.run(["npm", "run", "lint"], cwd=sample_dir, silent=True)

    l.ok()


# -----------------------------------------------------------------------------
def get_target_data_for_platform(platform):
    if platform == "wasm":
        return c.targets["wasm"]

    if platform:
        l.e(f"Invalid platform: {platform}")
    else:
        l.e("Define a valid platform")
