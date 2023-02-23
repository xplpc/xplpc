#! /usr/bin/env python3
# -*- coding: utf-8 -*-

"""
XPLPC MANAGER TOOL

Usage:
  xplpc.py <task-name>
  xplpc.py [options]
  xplpc.py -h | --help

Options:
  -h --help                         Show this screen.
  -d --debug                        Enable debug mode.
  --version                         Show version.

Examples:
  python3 xplpc.py -h
  python3 xplpc.py clean
  python3 xplpc.py format

Tasks:
  - clear
  - tree
  - brew

  - docs-format
  - python-format

  - cxx-format
  - cxx-build-static
  - cxx-build-shared
  - cxx-test
  - cxx-build-sample
  - cxx-run-sample
  - cxx-build-leaks

  - kotlin-format
  - kotlin-build
  - kotlin-test
  - kotlin-build-sample
  - kotlin-build-aar

  - swift-format
  - swift-build
  - swift-build-macos
  - swift-test
  - swift-build-xcframework
  - swift-build-xcframework-macos

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

  - flutter-build-xcframework
  - flutter-build-xcframework-macos
  - flutter-test
"""

import os

from docopt import docopt
from pygemstones.system import bootstrap as b
from pygemstones.util import log as l

import core.c as c
import core.config as cfg
import core.cxx as cxx
import core.docs as docs
import core.flutter as flutter
import core.general as general
import core.kotlin as kotlin
import core.python as python
import core.swift as swift
import core.wasm as wasm
from core import util


def main(options):
    cfg.proj_path = os.path.dirname(os.path.abspath(__file__))

    # show all params for debug
    if ("--debug" in options and options["--debug"]) or (
        "-d" in options and options["-d"]
    ):
        cfg.debug = True

    if cfg.debug:
        l.bold("You have executed with options:", l.YELLOW)
        l.m(str(options))
        l.nl()

    # bind options
    if "<task-name>" in options:
        task = options["<task-name>"]

    # validate task
    if not task:
        l.e("Task is invalid. Use 'python3 xplpc.py -h' for help.")

    #######################
    # GENERAL
    #######################

    # clear
    if task == "clear":
        general.run_task_clear()

    # tree
    elif task == "tree":
        general.run_task_tree()

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
    # PYTHON
    #######################

    # format
    elif task == "python-format":
        python.run_task_format()

    #######################
    # CXX
    #######################

    # format
    elif task == "cxx-format":
        cxx.run_task_format()

    # build static
    elif task == "cxx-build-static":
        cxx.run_task_build_static()

    # build shared
    elif task == "cxx-build-shared":
        cxx.run_task_build_shared()

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

    # build
    elif task == "kotlin-build":
        kotlin.run_task_build()

    # test
    elif task == "kotlin-test":
        kotlin.run_task_test()

    # build sample
    elif task == "kotlin-build-sample":
        kotlin.run_task_build_sample()

    # build aar
    elif task == "kotlin-build-aar":
        kotlin.run_task_build_aar()

    #######################
    # SWIFT
    #######################

    # format
    elif task == "swift-format":
        swift.run_task_format()

    # build
    elif task == "swift-build":
        swift.run_task_build()

    # build for macos
    elif task == "swift-build-macos":
        swift.run_task_build_macos()

    # test
    elif task == "swift-test":
        swift.run_task_test()

    # build xcframework
    elif task == "swift-build-xcframework":
        swift.run_task_build_xcframework()

    # build xcframework for macos
    elif task == "swift-build-xcframework-macos":
        swift.run_task_build_xcframework_macos()

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

    # build xcframework
    elif task == "flutter-build-xcframework":
        flutter.run_task_build_xcframework()

    # build xcframework for macos
    elif task == "flutter-build-xcframework-macos":
        flutter.run_task_build_xcframework_macos()

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
