import os

from pygemstones.system import platform as p
from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import config as c
from core import tool


# -----------------------------------------------------------------------------
def run_task_setup():
    # check
    tool.check_tool_conan()

    # create default profile
    l.i("Creating default profile...")

    r.run(
        [
            "conan",
            "profile",
            "new",
            "default",
            "--detect",
            "--force",
        ],
        cwd=c.proj_path,
    )

    # install darwin toolchain
    if c.conan_install_darwin_toolchain and p.is_macos():
        l.i("Installing darwin toolchain...")

        r.run(
            ["conan", "create", ".", "xplpc/stable"],
            cwd=os.path.join(
                c.proj_path,
                "conan",
                "darwin-toolchain",
            ),
        )

    l.ok()


# -----------------------------------------------------------------------------
def get_build_profile():
    if p.is_linux():
        return c.conan_build_profile_linux
    elif p.is_windows():
        return c.conan_build_profile_windows
    elif p.is_macos():
        return c.conan_build_profile_macos
    else:
        raise Exception("Build host system is unknown")


# -----------------------------------------------------------------------------
def add_target_setup_common_args(run_args, target_data, build_type):
    run_args.append("-s:h")
    run_args.append("build_type={0}".format(build_type))

    run_args.append("-s:h")
    run_args.append("arch={0}".format(target_data["conan_arch"]))

    if "enable_bitcode" in target_data:
        run_args.append("-o:h")
        run_args.append(
            "darwin-toolchain:enable_bitcode={0}".format(target_data["enable_bitcode"])
        )

    if "enable_arc" in target_data:
        run_args.append("-o:h")
        run_args.append(
            "darwin-toolchain:enable_arc={0}".format(target_data["enable_arc"])
        )

    if "enable_visibility" in target_data:
        run_args.append("-o:h")
        run_args.append(
            "darwin-toolchain:enable_visibility={0}".format(
                target_data["enable_visibility"]
            )
        )

    if "subsystem_ios_version" in target_data:
        run_args.append("-s:h")
        run_args.append(
            "os.subsystem.ios_version={0}".format(target_data["subsystem_ios_version"])
        )
    else:
        run_args.append("-s:h")
        run_args.append("os.version={0}".format(target_data["deployment_target"]))

    if "sdk" in target_data:
        run_args.append("-s:h")
        run_args.append("os.sdk={0}".format(target_data["sdk"]))

    if c.serializer == "json":
        run_args.append("-o")
        run_args.append("xplpc_enable_serializer_for_json={0}".format(True))
