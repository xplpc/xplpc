import os

from pygemstones.io import file as f
from pygemstones.util import log as l

from core import conan, net, run, tool, util
from core import config as c


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

    build_type = util.get_param_build_type(target, format="cmake")
    l.i(f"Build type: {build_type}")

    incremental = util.get_param_incremental()
    l.i(f"Incremental: {incremental}")

    target_data = get_target_data_for_platform("wasm")

    build_dir = os.path.join(c.proj_path, "build", target)
    conan_build_dir = os.path.join(c.proj_path, "build", "conan", target)

    if not incremental:
        f.recreate_dir(build_dir)

    # dependencies
    no_deps = util.get_param_no_deps()

    if not incremental and not no_deps and c.dependency_tool == "conan":
        for item in target_data:
            l.i(f"Building dependencies for arch {item['arch']}...")

            arch_dir = os.path.join(conan_build_dir, item["arch"])
            f.recreate_dir(arch_dir)

            conan.install(item, build_type, arch_dir)

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
            f"-DXPLPC_SANITIZER={util.get_param_sanitizer()}",
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

        run.run(run_args)

        # build
        run.run(["cmake", "--build", arch_dir])

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
    incremental = util.get_param_incremental()
    l.i(f"Incremental: {incremental}")

    if not incremental:
        l.i("Installing dependencies...")
        run.run(["npm", "ci"], cwd=sample_dir)

    # build
    l.i("Building...")
    run.run(["npm", "run", "build"], cwd=sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_run_sample():
    # check
    tool.check_tool_npm()
    tool.check_tool_node()

    # paths
    sample_dir = os.path.join(c.proj_path, "wasm", "sample")

    # dependencies
    incremental = util.get_param_incremental()
    l.i(f"Incremental: {incremental}")

    if not incremental:
        l.i("Installing dependencies...")
        run.run(["npm", "ci"], cwd=sample_dir)

    # build
    l.i("Building...")
    run.run(["npm", "run", "dev"], cwd=sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_serve_sample():
    # The sample is built for the path it is published under, so it is served under the same one rather than at the root, where every asset it asks for would be missing.

    dist_dir = os.path.join(c.proj_path, "wasm", "sample", "dist")

    if not f.dir_exists(dist_dir):
        l.e("The sample was not built, build it with: wasm-build-sample")

    serve_dir = os.path.join(c.proj_path, "build", "wasm-serve")
    base_dir = os.path.join(serve_dir, c.wasm_base_url.strip("/"))

    f.recreate_dir(serve_dir)
    f.copy_dir(dist_dir, base_dir)

    l.i(
        f"Serving at http://{c.http_server_host}:{c.http_server_port}{c.wasm_base_url}/"
    )

    net.serve(serve_dir)
    l.ok()


# -----------------------------------------------------------------------------
def run_task_test():
    # check
    tool.check_tool_npm()

    # paths
    sample_dir = os.path.join(c.proj_path, "wasm", "sample")

    # dependencies
    incremental = util.get_param_incremental()
    l.i(f"Incremental: {incremental}")

    if not incremental:
        l.i("Installing dependencies...")
        run.run(["npm", "ci"], cwd=sample_dir)

    # test
    l.i("Testing...")
    run.run(["npm", "run", "test:unit"], cwd=sample_dir)

    l.ok()


# -----------------------------------------------------------------------------
def run_task_format():
    # check
    tool.check_tool_npm()

    # paths
    sample_dir = os.path.join(c.proj_path, "wasm", "sample")

    # format js/css/html
    l.i("Formatting Web files...")
    run.run(["npm", "ci"], cwd=sample_dir, silent=True)
    run.run(["npm", "run", "lint"], cwd=sample_dir, silent=True)

    l.ok()


# -----------------------------------------------------------------------------
def get_target_data_for_platform(platform):
    if platform == "wasm":
        return c.targets["wasm"]

    if platform:
        l.e(f"Invalid platform: {platform}")
    else:
        l.e("Define a valid platform")
