import os

from pygemstones.util import log as l

from core import config as c
from core import run, tool

# The build profile is the one `conan profile detect` writes, and every host uses it.
BUILD_PROFILE = "default"


# -----------------------------------------------------------------------------
def run_task_setup():
    # check
    tool.check_tool_conan()

    # create default profile
    l.i("Creating default profile...")

    run.run(
        [
            "conan",
            "profile",
            "detect",
            "--force",
        ],
        cwd=c.proj_path,
    )

    # A profile that names a setting this conan does not have fails at the first build that uses it, and six of the nine are built by no job.
    l.i("Validating profiles...")

    profile_dir = os.path.join(c.proj_path, "conan", "profiles")

    for name in sorted(os.listdir(profile_dir)):
        run.run(
            [
                "conan",
                "profile",
                "show",
                "--profile:host",
                os.path.join(profile_dir, name),
            ],
            cwd=c.proj_path,
            silent=True,
        )

    l.ok()


# -----------------------------------------------------------------------------
def install(item, build_type, arch_dir, with_tests=False):
    run_args = [
        "conan",
        "install",
        c.proj_path,
        "-pr:b",
        BUILD_PROFILE,
        "-pr:h",
        os.path.join(c.proj_path, "conan", "profiles", item["conan_profile"]),
    ]

    add_target_setup_common_args(run_args, item, build_type, arch_dir)

    if with_tests:
        run_args.append("-o:h")
        run_args.append("xplpc_build_tests=True")

    run_args.append("--build=missing")
    run_args.append("--update")

    run.run(run_args, cwd=arch_dir)


# -----------------------------------------------------------------------------
def add_target_setup_common_args(run_args, target_data, build_type, output_folder):
    # conan 2 writes the generators next to the recipe unless the output folder is given
    run_args.append("--output-folder")
    run_args.append(output_folder)

    # build type
    run_args.append("-s:h")
    run_args.append(f"build_type={build_type}")

    # arch
    run_args.append("-s:h")
    run_args.append("arch={0}".format(target_data["conan_arch"]))

    # apple toolchain settings are configuration entries, not recipe options
    for name in ["enable_arc", "enable_bitcode", "enable_visibility"]:
        if name in target_data:
            run_args.append("-c:h")
            run_args.append(f"tools.apple:{name}={target_data[name]}")

    # sub system or system version
    if "subsystem_ios_version" in target_data:
        run_args.append("-s:h")
        run_args.append(
            "os.subsystem.ios_version={0}".format(target_data["subsystem_ios_version"])
        )
    elif "deployment_target" in target_data:
        run_args.append("-s:h")
        run_args.append("os.version={0}".format(target_data["deployment_target"]))

    # only the embedded apple platforms carry an sdk setting, macos does not have one
    if "sdk" in target_data and target_data["sdk"] != "macosx":
        run_args.append("-s:h")
        run_args.append("os.sdk={0}".format(target_data["sdk"]))

    # android api level
    if "api_level" in target_data:
        run_args.append("-s:h")
        run_args.append("os.api_level={0}".format(target_data["api_level"]))
