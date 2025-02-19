from conan import ConanFile


class TargetConan(ConanFile):
    name = "xplpc"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "xplpc_enable_serializer_for_json": [True, False],
        "xplpc_build_tests": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "xplpc_build_tests": False,
        "xplpc_enable_serializer_for_json": True,
    }
    generators = "CMakeDeps", "CMakeToolchain"

    # -----------------------------------------------------------------------------
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    # -----------------------------------------------------------------------------
    def requirements(self):
        self.requires("spdlog/1.15.0")

        if self.options.get_safe("xplpc_enable_serializer_for_json"):
            self.requires("nlohmann_json/3.11.3")

        if self.options.get_safe("xplpc_build_tests"):
            self.requires("gtest/1.15.0")
