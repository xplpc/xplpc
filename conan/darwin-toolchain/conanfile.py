from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import XCRun, is_apple_os

from conan import ConanFile

required_conan_version = ">=1.55.0"


class DarwinToolchainConan(ConanFile):
    name = "darwin-toolchain"
    version = "1.0.0"
    license = "MIT"
    settings = "os", "arch", "build_type", "os_build", "compiler"
    options = {
        "enable_bitcode": [True, False, None],
        "enable_arc": [True, False, None],
        "enable_visibility": [True, False, None],
    }
    default_options = {
        "enable_bitcode": None,
        "enable_arc": None,
        "enable_visibility": None,
    }
    description = "Darwin toolchain to (cross) compile macOS/iOS/watchOS/tvOS"
    url = "https://github.com/xplpc/xplpc"
    build_policy = "missing"

    def config_options(self):
        if self.settings.os == "Macos":
            self.options.enable_bitcode = None

        if self.settings.os == "watchOS":
            self.options.enable_bitcode = True

        if self.settings.os == "tvOS":
            self.options.enable_bitcode = True

    def validate(self):
        if self.settings.os_build != "Macos":
            raise ConanInvalidConfiguration("Build machine must be Macos")

        if not is_apple_os(self):
            raise ConanInvalidConfiguration("OS must be an Apple OS")

        if self.settings.os in ["watchOS", "tvOS"] and not self.options.enable_bitcode:
            raise ConanInvalidConfiguration("Bitcode is required on watchOS/tvOS")

        if self.settings.os == "Macos" and self.settings.arch not in [
            "x86",
            "x86_64",
            "armv8",
        ]:
            raise ConanInvalidConfiguration(
                "macOS: Only supported archs: [x86, x86_64, armv8]"
            )

        if self.settings.os == "iOS" and self.settings.arch not in [
            "armv7",
            "armv7s",
            "armv8",
            "armv8.3",
            "x86",
            "x86_64",
        ]:
            raise ConanInvalidConfiguration(
                "iOS: Only supported archs: [armv7, armv7s, armv8, armv8.3, x86, x86_64]"
            )

        if self.settings.os == "tvOS" and self.settings.arch not in ["armv8", "x86_64"]:
            raise ConanInvalidConfiguration(
                "tvOS: Only supported archs: [armv8, x86_64]"
            )

        if self.settings.os == "watchOS" and self.settings.arch not in [
            "armv7k",
            "armv8_32",
            "x86",
            "x86_64",
        ]:
            raise ConanInvalidConfiguration(
                "watchOS: Only supported archs: [armv7k, armv8_32, x86, x86_64]"
            )

    def package_info(self):
        self.cpp_info.libdirs = []
        self.cpp_info.bindirs = []
        self.cpp_info.includedirs = []
        self.cpp_info.resdirs = []
        self.cpp_info.frameworkdirs = []

        # Sysroot and Settings
        xcrun = XCRun(self, use_settings_target=True)
        sysroot = xcrun.sdk_path
        settings_target = xcrun.settings

        self.cpp_info.sysroot = sysroot
        common_flags = ["-isysroot%s" % sysroot]

        self.env_info.CONAN_CMAKE_OSX_SYSROOT = sysroot
        self.env_info.SDKROOT = sysroot

        self.buildenv_info.define("CONAN_CMAKE_OSX_SYSROOT", sysroot)
        self.buildenv_info.define("SDKROOT", sysroot)
        self.runenv_info.define("CONAN_CMAKE_OSX_SYSROOT", sysroot)
        self.runenv_info.define("SDKROOT", sysroot)
        self.conf_info.define("tools.apple:sdk_path", sysroot)

        # Bitcode
        if self.options.enable_bitcode is None or self.options.enable_bitcode == "None":
            self.output.info("Bitcode enabled: IGNORED")
        else:
            if self.options.enable_bitcode:
                self.output.info("Bitcode enabled: YES")

                self.env_info.CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE = "YES"
                self.buildenv_info.define("CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE", "YES")
                self.runenv_info.define("CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE", "YES")

                build_type = settings_target.get_safe("build_type")

                if build_type.lower() == "debug":
                    self.env_info.CMAKE_XCODE_ATTRIBUTE_BITCODE_GENERATION_MODE = (
                        "marker"
                    )
                    self.buildenv_info.define(
                        "CMAKE_XCODE_ATTRIBUTE_BITCODE_GENERATION_MODE", "marker"
                    )
                    self.runenv_info.define(
                        "CMAKE_XCODE_ATTRIBUTE_BITCODE_GENERATION_MODE", "marker"
                    )
                    common_flags.append("-fembed-bitcode-marker")
                else:
                    self.env_info.CMAKE_XCODE_ATTRIBUTE_BITCODE_GENERATION_MODE = (
                        "bitcode"
                    )
                    self.buildenv_info.define(
                        "CMAKE_XCODE_ATTRIBUTE_BITCODE_GENERATION_MODE", "bitcode"
                    )
                    self.runenv_info.define(
                        "CMAKE_XCODE_ATTRIBUTE_BITCODE_GENERATION_MODE", "bitcode"
                    )
                    common_flags.append("-fembed-bitcode")
            else:
                self.output.info("Bitcode enabled: NO")

            self.conf_info.define(
                "tools.apple:enable_bitcode", self.options.enable_bitcode
            )

        # ARC
        if self.options.enable_arc is None or self.options.enable_arc == "None":
            self.output.info("ObjC ARC enabled: IGNORED")
        else:
            if self.options.enable_arc:
                common_flags.append("-fobjc-arc")
                self.env_info.CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC = "YES"
                self.buildenv_info.define(
                    "CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC", "YES"
                )
                self.runenv_info.define(
                    "CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC", "YES"
                )
                self.output.info("ObjC ARC enabled: YES")
            else:
                common_flags.append("-fno-objc-arc")
                self.env_info.CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC = "NO"
                self.buildenv_info.define(
                    "CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC", "NO"
                )
                self.runenv_info.define(
                    "CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC", "NO"
                )
                self.output.info("ObjC ARC enabled: NO")

            self.conf_info.define("tools.apple:enable_arc", self.options.enable_arc)

        # Visibility
        if (
            self.options.enable_visibility is None
            or self.options.enable_visibility == "None"
        ):
            self.output.info("Visibility enabled: IGNORED")
        else:
            if self.options.enable_visibility:
                common_flags.append("-fvisibility=default")
                self.env_info.CMAKE_XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN = "NO"
                self.buildenv_info.define(
                    "CMAKE_XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN", "NO"
                )
                self.runenv_info.define(
                    "CMAKE_XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN", "NO"
                )
                self.output.info("Visibility enabled: YES")
            else:
                common_flags.append("-fvisibility=hidden")
                self.env_info.CMAKE_XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN = "YES"
                self.buildenv_info.define(
                    "CMAKE_XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN", "YES"
                )
                self.runenv_info.define(
                    "CMAKE_XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN", "YES"
                )
                self.output.info("Visibility enabled: NO")

            self.conf_info.define(
                "tools.apple:enable_visibility", self.options.enable_visibility
            )

        self.cpp_info.cflags.extend(common_flags)
        self.cpp_info.cxxflags.extend(common_flags)
        self.cpp_info.sharedlinkflags.extend(common_flags)
        self.cpp_info.exelinkflags.extend(common_flags)

        cflags_str = " ".join(self.cpp_info.cflags)
        cxxflags_str = " ".join(self.cpp_info.cxxflags)
        ldflags_str = " ".join(self.cpp_info.sharedlinkflags)

        self.env_info.CFLAGS = cflags_str
        self.env_info.ASFLAGS = cflags_str
        self.env_info.CPPFLAGS = cxxflags_str
        self.env_info.CXXFLAGS = cxxflags_str
        self.env_info.LDFLAGS = ldflags_str

        self.buildenv_info.define("CFLAGS", cflags_str)
        self.buildenv_info.define("ASFLAGS", cflags_str)
        self.buildenv_info.define("CPPFLAGS", cxxflags_str)
        self.buildenv_info.define("CXXFLAGS", cxxflags_str)
        self.buildenv_info.define("LDFLAGS", ldflags_str)
