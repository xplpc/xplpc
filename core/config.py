from core.targets import kotlin as target_kotlin
from core.targets import swift_ios as target_swift_ios
from core.targets import swift_ios_flutter as target_swift_ios_flutter
from core.targets import swift_macos as target_swift_macos
from core.targets import swift_macos_flutter as target_swift_macos_flutter
from core.targets import swift_test as target_swift_test

# general
debug = False
task = ""
proj_path = ""
options = {}

# build type
build_type = "Release"
build_type_kotlin = "RelWithDebInfo"

# dependency (cpm, conan)
dependency_tool = "conan"

# serializer
serializer = "json"

# http
http_server_host = "127.0.0.1"
http_server_port = "8000"

# wasm
wasm_base_url = "/wasm-demo"

# targets
targets = {}
targets["swift-ios"] = target_swift_ios.data
targets["swift-macos"] = target_swift_macos.data
targets["swift-ios-flutter"] = target_swift_ios_flutter.data
targets["swift-macos-flutter"] = target_swift_macos_flutter.data
targets["swift-test"] = target_swift_test.data
targets["kotlin"] = target_kotlin.data

# conan
conan_use_darwin_toolchain = False

conan_build_profile_macos = "default"
conan_build_profile_linux = "default"
conan_build_profile_windows = "default"
