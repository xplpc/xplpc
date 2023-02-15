import os

from pygemstones.io import file as f
from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import config as c
from core import tool, util
from core import swift


# -----------------------------------------------------------------------------
def run_task_build_xcframework():
    # check
    tool.check_tool_cmake()
    tool.check_tool_xcodebuild()

    # environment
    os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # build libraries
    l.i("Building native library for iOS...")

    swift.do_build(
        build_dir_name="flutter-ios",
        framework_list=c.swift_framework_list,
        has_interface=True,
    )

    # build xcframework
    l.i("Building xcframework for iOS...")

    swift.do_build_xcframework(
        build_dir_name="flutter-ios",
        build_dir_prefix="flutter-ios",
        framework_list=c.swift_framework_list,
    )

    l.ok()


# -----------------------------------------------------------------------------
def run_task_build_xcframework_macos():
    # check
    tool.check_tool_cmake()
    tool.check_tool_xcodebuild()

    # environment
    os.environ["CPM_SOURCE_CACHE"] = os.path.join(f.home_dir(), ".cache", "CPM")

    # build libraries
    l.i("Building native library for macOS...")

    swift.do_build(
        build_dir_name="flutter-macos",
        framework_list=c.swift_framework_list_for_macos,
        has_interface=True,
    )

    # build xcframework
    l.i("Building xcframework for macOS...")

    swift.do_build_xcframework(
        build_dir_name="flutter-macos",
        build_dir_prefix="flutter-macos",
        framework_list=c.swift_framework_list_for_macos,
    )

    l.ok()
