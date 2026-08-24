#! /usr/bin/env python3

"""
XPLPC MANAGER TOOL

Usage:
  xplpc.py <task-name> [options]
  xplpc.py [options]
  xplpc.py -h | --help

Options:
  -h --help                         Show this screen.
  -d --debug                        Enable debug mode.
  --version                         Show version.
  --incremental                     Keep what a previous build left, skipping the setup steps.
  --build=<build-type>              Build type.
  --interface                       Enable C interface.
  --platform=<platform>             Define custom platform.
  --no-deps                         Run without build dependencies.
  --sanitizer=<sanitizer>           Build with a sanitizer (address, thread, undefined).
  --dependency-tool=<tool>          Resolve dependencies with cpm or conan.
  --url=<url>                       Define the url an artifact will be published to, defaulting to the release named after the project version.
  --wheel                           Install the built wheel instead of the development package.

Examples:
  python3 xplpc.py -h
  python3 xplpc.py clean
  python3 xplpc.py format

Tasks:
  - clear
  - brew

  - docs-format
  - conan-setup

  - cxx-format
  - cxx-build-static
  - cxx-test
  - cxx-build-sample
  - cxx-run-sample
  - cxx-build-leaks

  - kotlin-format
  - kotlin-lint
  - kotlin-build
  - kotlin-test
  - kotlin-build-sample
  - kotlin-run-sample
  - kotlin-build-aar
  - kotlin-build-jar

  - swift-format
  - swift-lint
  - swift-build
  - swift-test
  - swift-test (--platform ios)
  - swift-test (--platform ios-device)
  - swift-build-xcframework
  - swift-build-sample
  - swift-generate-sample-project
  - swift-generate-spm-package [--url]

  - wasm-format
  - wasm-build
  - wasm-test
  - wasm-build-sample
  - wasm-run-sample
  - wasm-serve-sample

  - c-format
  - c-build-static
  - c-build-shared
  - c-test
  - c-build-sample
  - c-run-sample
  - c-build-leaks

  - flutter-format
  - flutter-test

  - python-format
  - python-lint
  - python-build
  - python-install [--wheel]
  - python-test
  - python-run-sample
  - python-pyinstaller
"""

import os

from docopt import docopt
from pygemstones.system import bootstrap as b
from pygemstones.util import log as l

import core.config as cfg
from core import (
    c,
    conan,
    cxx,
    docs,
    flutter,
    general,
    kotlin,
    python,
    swift,
    util,
    wasm,
)


def main(options):
    cfg.proj_path = os.path.dirname(os.path.abspath(__file__))

    # show all params for debug
    if (options.get("--debug")) or (options.get("-d")):
        l.bold("You have executed with options:", l.YELLOW)
        l.m(str(options))
        l.nl()

    # bind options
    cfg.options = options

    if options["--dependency-tool"]:
        cfg.dependency_tool = util.get_param_dependency_tool()

    # validate task
    task = options["<task-name>"]

    if not task:
        l.e("Task is invalid. Use 'python3 xplpc.py -h' for help.")

    #######################
    # GENERAL
    #######################

    # clear
    if task == "clear":
        general.run_task_clear()

    # brew
    elif task == "brew":
        general.run_task_brew()

    #######################
    # DOCS
    #######################

    # format
    elif task == "docs-format":
        docs.run_task_format()

    #######################
    # CONAN
    #######################

    # setup
    elif task == "conan-setup":
        conan.run_task_setup()

    #######################
    # PYTHON
    #######################

    # format
    elif task == "python-format":
        python.run_task_format()

    # lint
    elif task == "python-lint":
        python.run_task_lint()

    # build
    elif task == "python-build":
        python.run_task_build()

    # install
    elif task == "python-install":
        python.run_task_install()

    # test
    elif task == "python-test":
        python.run_task_test()

    # run sample
    elif task == "python-run-sample":
        python.run_task_run_sample()

    # pyinstaller
    elif task == "python-pyinstaller":
        python.run_task_pyinstaller()

    #######################
    # CXX
    #######################

    # format
    elif task == "cxx-format":
        cxx.run_task_format()

    # build static
    elif task == "cxx-build-static":
        cxx.run_task_build_static()

    # test
    elif task == "cxx-test":
        cxx.run_task_test()

    # build sample
    elif task == "cxx-build-sample":
        cxx.run_task_build_sample()

    # run sample
    elif task == "cxx-run-sample":
        cxx.run_task_run_sample()

    # build leaks
    elif task == "cxx-build-leaks":
        cxx.run_task_build_leaks()

    #######################
    # KOTLIN
    #######################

    # format
    elif task == "kotlin-format":
        kotlin.run_task_format()

    # lint
    elif task == "kotlin-lint":
        kotlin.run_task_lint()

    # build
    elif task == "kotlin-build":
        kotlin.run_task_build()

    # test
    elif task == "kotlin-test":
        kotlin.run_task_test()

    # build sample
    elif task == "kotlin-build-sample":
        kotlin.run_task_build_sample()

    # run sample
    elif task == "kotlin-run-sample":
        kotlin.run_task_run_sample()

    # build aar
    elif task == "kotlin-build-aar":
        kotlin.run_task_build_aar()

    # build jar
    elif task == "kotlin-build-jar":
        kotlin.run_task_build_jar()

    #######################
    # SWIFT
    #######################

    # format
    elif task == "swift-format":
        swift.run_task_format()

    # lint
    elif task == "swift-lint":
        swift.run_task_lint()

    # build
    elif task == "swift-build":
        swift.run_task_build()

    # test
    elif task == "swift-test":
        swift.run_task_test()

    # build xcframework
    elif task == "swift-build-xcframework":
        swift.run_task_build_xcframework()

    # build sample
    elif task == "swift-build-sample":
        swift.run_task_build_sample()

    # generate sample project
    elif task == "swift-generate-sample-project":
        swift.run_task_generate_sample_project()

    # generate swift package
    elif task == "swift-generate-spm-package":
        swift.run_task_generate_spm_package()

    #######################
    # WASM
    #######################

    # format
    elif task == "wasm-format":
        wasm.run_task_format()

    # build
    elif task == "wasm-build":
        wasm.run_task_build()

    # test
    elif task == "wasm-test":
        wasm.run_task_test()

    # build sample
    elif task == "wasm-build-sample":
        wasm.run_task_build_sample()

    # run sample for development
    elif task == "wasm-run-sample":
        wasm.run_task_run_sample()

    # serve sample
    elif task == "wasm-serve-sample":
        wasm.run_task_serve_sample()

    #######################
    # C
    #######################

    # format
    elif task == "c-format":
        c.run_task_format()

    # build static
    elif task == "c-build-static":
        c.run_task_build_static()

    # build shared
    elif task == "c-build-shared":
        c.run_task_build_shared()

    # test
    elif task == "c-test":
        c.run_task_test()

    # build sample
    elif task == "c-build-sample":
        c.run_task_build_sample()

    # run sample
    elif task == "c-run-sample":
        c.run_task_run_sample()

    # build leaks
    elif task == "c-build-leaks":
        c.run_task_build_leaks()

    #######################
    # FLUTTER
    #######################

    # format
    elif task == "flutter-format":
        flutter.run_task_format()

    # test
    elif task == "flutter-test":
        flutter.run_task_test()

    #######################
    # INVALID
    #######################

    # task is invalid
    else:
        l.e("Task is invalid")


if __name__ == "__main__":
    # initialization
    util.remove_sdkroot_from_env()
    b.init()

    # main CLI entrypoint
    args = docopt(__doc__, version="1.0.0")
    main(args)
