from conan.tools.build import check_min_cppstd

from conan import ConanFile


class TargetConan(ConanFile):
    name = "xplpc"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "xplpc_build_tests": [True, False],
    }
    default_options = {
        "xplpc_build_tests": False,
    }
    generators = "CMakeDeps", "CMakeToolchain"

    def validate(self):
        check_min_cppstd(self, 20)

    def requirements(self):
        self.requires("spdlog/1.17.0")
        self.requires("nlohmann_json/3.12.0")

        if self.options.get_safe("xplpc_build_tests"):
            self.requires("gtest/1.18.0")
