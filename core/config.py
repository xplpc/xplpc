# general
debug = False
task = ""
proj_path = ""
options = {}

build_type = "Release"
build_type_kotlin = "RelWithDebInfo"

# swift
swift_framework_list_for_ios = [
    {
        "arch": "ios-arm64",
        "platform": "OS64",
        "deployment_target": "11.0",
        "sdk_version": "11.0",
        "group": "ios",
    },
    {
        "arch": "ios-sim-arm64",
        "platform": "SIMULATORARM64",
        "deployment_target": "11.0",
        "sdk_version": "11.0",
        "group": "ios-sim",
    },
    {
        "arch": "ios-sim-x64",
        "platform": "SIMULATOR64",
        "deployment_target": "11.0",
        "sdk_version": "11.0",
        "group": "ios-sim",
    },
    {
        "arch": "tvos-arm64",
        "platform": "TVOS",
        "deployment_target": "11.0",
        "sdk_version": "11.0",
        "group": "tvos",
    },
    {
        "arch": "tvos-sim-x64",
        "platform": "SIMULATOR_TVOS",
        "deployment_target": "11.0",
        "sdk_version": "11.0",
        "group": "tvos-sim",
    },
    {
        "arch": "watchos-arm64",
        "platform": "WATCHOS",
        "deployment_target": "4.0",
        "sdk_version": "11.0",
        "group": "watchos",
    },
    {
        "arch": "watchos-sim-x64",
        "platform": "SIMULATOR_WATCHOS",
        "deployment_target": "4.0",
        "sdk_version": "11.0",
        "group": "watchos-sim",
    },
    {
        "arch": "catalyst-x64",
        "platform": "MAC_CATALYST",
        "deployment_target": "13.1",
        "sdk_version": "11.0",
        "group": "catalyst",
    },
    {
        "arch": "catalyst-arm64",
        "platform": "MAC_CATALYST_ARM64",
        "deployment_target": "13.1",
        "sdk_version": "11.0",
        "group": "catalyst",
    },
]

swift_framework_list_for_ios_flutter = [
    {
        "arch": "ios-arm64",
        "platform": "OS64",
        "deployment_target": "11.0",
        "sdk_version": "11.0",
        "group": "ios",
    },
    {
        "arch": "ios-sim-arm64",
        "platform": "SIMULATORARM64",
        "deployment_target": "11.0",
        "sdk_version": "11.0",
        "group": "ios-sim",
    },
    {
        "arch": "ios-sim-x64",
        "platform": "SIMULATOR64",
        "deployment_target": "11.0",
        "sdk_version": "11.0",
        "group": "ios-sim",
    },
]

swift_framework_list_for_macos = [
    {
        "arch": "macos-x64",
        "platform": "MAC",
        "deployment_target": "10.13",
        "sdk_version": "11.0",
        "group": "macos",
    },
    {
        "arch": "macos-arm64",
        "platform": "MAC_ARM64",
        "deployment_target": "11.0",
        "sdk_version": "11.0",
        "group": "macos",
    },
]

swift_framework_list_for_macos_flutter = [
    {
        "arch": "macos-x64",
        "platform": "MAC",
        "deployment_target": "10.13",
        "sdk_version": "11.0",
        "group": "macos",
    },
    {
        "arch": "macos-arm64",
        "platform": "MAC_ARM64",
        "deployment_target": "11.0",
        "sdk_version": "11.0",
        "group": "macos",
    },
]

swift_test_list = [
    {
        "arch": "macos-arm64",
    },
]

# http
http_server_host = "127.0.0.1"
http_server_port = "8000"

# wasm
wasm_base_url = "/wasm-demo"
