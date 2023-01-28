# general
debug = False
task = ""
proj_path = ""

# TODO: XPLPC - Return to Release
build_type = "Debug"

# swift
swift_framework_list = [
    {
        "arch": "ios-arm64",
        "platform": "OS64",
        "version": "11.0",
        "group": "ios",
    },
    {
        "arch": "ios-sim-arm64",
        "platform": "SIMULATORARM64",
        "version": "11.0",
        "group": "ios-sim",
    },
    {
        "arch": "ios-sim-x64",
        "platform": "SIMULATOR64",
        "version": "11.0",
        "group": "ios-sim",
    },
    {
        "arch": "tvos-arm64",
        "platform": "TVOS",
        "version": "11.0",
        "group": "tvos",
    },
    {
        "arch": "tvos-sim-x64",
        "platform": "SIMULATOR_TVOS",
        "version": "11.0",
        "group": "tvos-sim",
    },
    {
        "arch": "watchos-arm64",
        "platform": "WATCHOS",
        "version": "4.0",
        "group": "watchos",
    },
    {
        "arch": "watchos-sim-x64",
        "platform": "SIMULATOR_WATCHOS",
        "version": "4.0",
        "group": "watchos-sim",
    },
    {
        "arch": "catalyst-x64",
        "platform": "MAC_CATALYST",
        "version": "13.1",
        "group": "catalyst",
    },
    {
        "arch": "catalyst-arm64",
        "platform": "MAC_CATALYST_ARM64",
        "version": "13.1",
        "group": "catalyst",
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
