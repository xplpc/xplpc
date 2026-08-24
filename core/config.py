from core.targets import kotlin_android as target_kotlin_android
from core.targets import kotlin_desktop as target_kotlin_desktop
from core.targets import kotlin_flutter as target_kotlin_flutter
from core.targets import platform_linux as target_platform_linux
from core.targets import platform_macos as target_platform_macos
from core.targets import platform_windows as target_platform_windows
from core.targets import swift_apple as target_swift_apple
from core.targets import swift_ios as target_swift_ios
from core.targets import swift_ios_flutter as target_swift_ios_flutter
from core.targets import swift_macos as target_swift_macos
from core.targets import swift_macos_flutter as target_swift_macos_flutter
from core.targets import swift_test as target_swift_test
from core.targets import wasm as target_wasm

# general
proj_path = ""
options = {}

# build type
build_type = "Release"
build_type_kotlin = "RelWithDebInfo"

# dependency (cpm, conan)
dependency_tool = "cpm"


# http
http_server_host = "127.0.0.1"
http_server_port = "8000"

# wasm
wasm_base_url = "/wasm-demo"

# targets
targets = {}
targets["swift-apple"] = target_swift_apple.data
targets["swift-ios"] = target_swift_ios.data
targets["swift-macos"] = target_swift_macos.data
targets["swift-ios-flutter"] = target_swift_ios_flutter.data
targets["swift-macos-flutter"] = target_swift_macos_flutter.data
targets["swift-test"] = target_swift_test.data
targets["kotlin-android"] = target_kotlin_android.data
targets["kotlin-desktop"] = target_kotlin_desktop.data
targets["kotlin-flutter"] = target_kotlin_flutter.data
targets["wasm"] = target_wasm.data
targets["platform-macos"] = target_platform_macos.data
targets["platform-windows"] = target_platform_windows.data
targets["platform-linux"] = target_platform_linux.data
