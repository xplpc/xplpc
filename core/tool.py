import os
import subprocess

from pygemstones.io import file as f
from pygemstones.system import platform as p
from pygemstones.util import log as l


# -----------------------------------------------------------------------------
def check_tool_gradlew(path):
    """Checks if gradlew exists."""
    gradle_tool = os.path.join(path, "gradlew")

    if p.is_windows():
        gradle_tool = gradle_tool + ".bat"

    if not f.file_exists(gradle_tool):
        l.e(f"Gradle script don't exists: {gradle_tool}")

    return gradle_tool


# -----------------------------------------------------------------------------
def check_tool_adb():
    """Checks if invoking supplied adb binary works."""
    try:
        subprocess.check_output(["adb", "--version"])
        return True
    except OSError:
        l.e(
            "The tool ADB is not installed, check: https://developer.android.com/studio/command-line/adb"
        )
        return False


# -----------------------------------------------------------------------------
def check_tool_xcodebuild():
    """Checks if invoking supplied xcodebuild binary works."""
    try:
        subprocess.check_output(["xcodebuild", "-version"])
        return True
    except OSError:
        l.e("Xcode is not installed, check: https://developer.apple.com/xcode")
        return False


# -----------------------------------------------------------------------------
def check_tool_ios_deploy():
    """Checks if invoking supplied ios-deploy binary works."""
    try:
        subprocess.check_output(["ios-deploy", "--version"])
        return True
    except OSError:
        l.e(
            "The tool ios-deploy is not installed, check: https://github.com/ios-control/ios-deploy"
        )
        return False


# -----------------------------------------------------------------------------
def check_tool_cocoapods():
    """Checks if invoking supplied pod binary works."""
    try:
        subprocess.check_output(["pod", "--version"])
        return True
    except OSError:
        l.e(
            "The tool cocoapods is not installed, check: https://github.com/CocoaPods/CocoaPods"
        )
        return False


# -----------------------------------------------------------------------------
def check_tool_cxx_formatter():
    try:
        subprocess.check_output(["clang-format", "--version"])
        return True
    except OSError:
        l.e(
            "Clang-format is not installed, check: https://clang.llvm.org/docs/ClangFormat.html"
        )
        return False


# -----------------------------------------------------------------------------
def check_tool_python_formatter():
    try:
        subprocess.check_output(["black", "--version"])
        return True
    except OSError:
        l.e("Black is not installed, check: https://github.com/psf/black")
        return False


# -----------------------------------------------------------------------------
def check_tool_cmake_formatter():
    try:
        subprocess.check_output(["cmake-format", "--version"])
        return True
    except OSError:
        l.e(
            "Cmake-format is not installed, check: https://github.com/cheshirekow/cmake_format"
        )
        return False


# -----------------------------------------------------------------------------
def check_tool_tree():
    try:
        subprocess.check_output(["tree", "--version"])
        return True
    except OSError:
        l.e("Tree tool is not installed")
        return False


# -----------------------------------------------------------------------------
def check_tool_brew():
    try:
        subprocess.check_output(["brew", "--version"])
        return True
    except OSError:
        l.e("Brew is not installed, check: https://brew.sh")
        return False


# -----------------------------------------------------------------------------
def check_tool_kotlin_formatter():
    try:
        subprocess.check_output(["ktlint", "--version"])
        return True
    except OSError:
        l.e("Ktlint is not installed, check: https://github.com/pinterest/ktlint")
        return False


# -----------------------------------------------------------------------------
def check_tool_swift_formatter():
    try:
        subprocess.check_output(["swiftformat", "--version"])
        return True
    except OSError:
        l.e(
            "SwiftFormat is not installed, check: https://github.com/nicklockwood/SwiftFormat"
        )
        return False


# -----------------------------------------------------------------------------
def check_tool_cmake():
    try:
        subprocess.check_output(["cmake", "--version"])
        return True
    except OSError:
        l.e("CMake is not installed, check: https://cmake.org")
        return False


# -----------------------------------------------------------------------------
def check_tool_leaks():
    if f.file_exists("/usr/bin/leaks"):
        return True

    l.e("Leaks tool is not installed, check: https://developer.apple.com/xcode")
    return False


# -----------------------------------------------------------------------------
def check_tool_emsdk():
    try:
        subprocess.check_output(["emsdk", "help"])

        if os.environ.get("EMSDK") is not None:
            return True

        return False
    except OSError:
        l.e(
            "Emscripten SDK is not installed, check: https://github.com/emscripten-core/emsdk"
        )
        return False


# -----------------------------------------------------------------------------
def check_tool_npm():
    try:
        subprocess.check_output(["npm", "--version"])
        return True
    except OSError:
        l.e("NPM is not installed, check: https://www.npmjs.com/")
        return False


# -----------------------------------------------------------------------------
def check_tool_node():
    try:
        subprocess.check_output(["node", "--version"])
        return True
    except OSError:
        l.e("Node is not installed, check: https://nodejs.org/")
        return False


# -----------------------------------------------------------------------------
def check_tool_remark():
    try:
        subprocess.check_output(["remark", "--version"])
        return True
    except OSError:
        l.e("Remark is not installed, check: https://github.com/remarkjs/remark")
        return False


# -----------------------------------------------------------------------------
def check_tool_flutter():
    try:
        subprocess.check_output(["flutter", "--version"])
        return True
    except OSError:
        l.e(
            "Flutter is not installed, check: https://docs.flutter.dev/get-started/install"
        )
        return False


# -----------------------------------------------------------------------------
def check_tool_conan():
    try:
        subprocess.check_output(["conan", "--version"])
        return True
    except OSError:
        l.e("Conan is not installed, check: https://github.com/conan-io/conan")
        return False


# -----------------------------------------------------------------------------
def check_tool_python():
    try:
        subprocess.check_output(["python3", "--version"])
        return True
    except OSError:
        l.e("Python is not installed, check: https://www.python.org")
        return False


# -----------------------------------------------------------------------------
def check_tool_pip():
    try:
        subprocess.check_output(["python3", "-m", "pip", "--version"])
        return True
    except OSError:
        l.e("PIP is not installed, check: https://pypi.org/project/pip")
        return False


# -----------------------------------------------------------------------------
def check_tool_pytest():
    try:
        subprocess.check_output(["pytest", "--version"])
        return True
    except OSError:
        l.e("PyTest is not installed, check: https://www.pytest.org")
        return False


# -----------------------------------------------------------------------------
def check_and_get_env(name):
    env = os.environ.get(name)

    if env:
        return env
    else:
        l.e(f"Environment variable is not set: {name}")
