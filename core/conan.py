import os

from pygemstones.io import file as f
from pygemstones.io import net as n
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
            "detect",
            "--force",
        ],
        cwd=c.proj_path,
    )

    # download files
    f.recreate_dir(os.path.join("build", "conan"))

    n.download(
        "https://raw.githubusercontent.com/conan-io/cmake-conan/develop2/conan_provider.cmake",
        os.path.join(
            "build",
            "conan",
            "conan_provider.cmake",
        ),
    )

    n.download(
        "https://raw.githubusercontent.com/conan-io/cmake-conan/develop2/conan_support.cmake",
        os.path.join(
            "build",
            "conan",
            "conan_support.cmake",
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
    # build type
    run_args.append("-s:h")
    run_args.append("build_type={0}".format(build_type))

    # arch
    run_args.append("-s:h")
    run_args.append("arch={0}".format(target_data["conan_arch"]))

    # arc
    if "enable_arc" in target_data:
        run_args.append("-o:h")

        if c.conan_use_darwin_toolchain:
            run_args.append(
                "darwin-toolchain:enable_arc={0}".format(target_data["enable_arc"])
            )
        else:
            run_args.append(
                "tools.apple:enable_arc={0}".format(target_data["enable_arc"])
            )

    # bitcode
    if "enable_bitcode" in target_data:
        run_args.append("-o:h")

        if c.conan_use_darwin_toolchain:
            run_args.append(
                "darwin-toolchain:enable_bitcode={0}".format(
                    target_data["enable_bitcode"]
                )
            )
        else:
            run_args.append(
                "tools.apple:enable_bitcode={0}".format(target_data["enable_bitcode"])
            )

    # visibility
    if "enable_visibility" in target_data:
        run_args.append("-o:h")

        if c.conan_use_darwin_toolchain:
            run_args.append(
                "darwin-toolchain:enable_visibility={0}".format(
                    target_data["enable_visibility"]
                )
            )
        else:
            run_args.append(
                "tools.apple:enable_visibility={0}".format(
                    target_data["enable_visibility"]
                )
            )

    # sub system or system version
    if "subsystem_ios_version" in target_data:
        run_args.append("-s:h")
        run_args.append(
            "os.subsystem.ios_version={0}".format(target_data["subsystem_ios_version"])
        )
    elif "deployment_target" in target_data:
        run_args.append("-s:h")
        run_args.append("os.version={0}".format(target_data["deployment_target"]))

    # ios sdk
    if "sdk" in target_data:
        run_args.append("-s:h")
        run_args.append("os.sdk={0}".format(target_data["sdk"]))

    # android api level
    if "api_level" in target_data:
        run_args.append("-s:h")
        run_args.append("os.api_level={0}".format(target_data["api_level"]))

    # serializer
    if c.serializer == "json":
        run_args.append("-o")
        run_args.append("xplpc_enable_serializer_for_json={0}".format(True))
