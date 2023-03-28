from conan import ConanFile


class TargetConan(ConanFile):
    name = "xplpc"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "xplpc_enable_serializer_for_json": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "xplpc_enable_serializer_for_json": True,
    }
    generators = "CMakeDeps", "CMakeToolchain"

    # -----------------------------------------------------------------------------
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    # -----------------------------------------------------------------------------
    def requirements(self):
        self.requires("spdlog/1.11.0")

        if self.options.get_safe("xplpc_enable_serializer_for_json", default=False):
            self.requires("nlohmann_json/3.11.2")
