# archs
has_ios_arm64 = True
has_ios_simulator_x64 = True
has_ios_simulator_arm64 = True
has_tvos_arm64 = True
has_tvos_simulator_x64 = True
has_tvos_simulator_arm64 = True
has_watchos_arm64 = True
has_watchos_simulator_x64 = True
has_watchos_simulator_arm64 = True
has_mac_catalyst_x64 = True
has_mac_catalyst_arm64 = True

data = []

# ios - arm64
if has_ios_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_ios_profile",
                "deployment_target": "15.0",
                "sdk": "iphoneos",
                "group": "ios",
                "expected_platform": "IOS",
                "platform": "OS64",
                "enable_arc": True,
                "enable_visibility": False,
            },
        ]
    )

# ios simulator - x64
if has_ios_simulator_x64:
    data.extend(
        [
            {
                "arch": "x86_64",
                "conan_arch": "x86_64",
                "conan_profile": "xplpc_ios_profile",
                "deployment_target": "15.0",
                "sdk": "iphonesimulator",
                "group": "ios-simulator",
                "expected_platform": "IOSSIMULATOR",
                "platform": "SIMULATOR64",
                "enable_arc": True,
                "enable_visibility": False,
            },
        ]
    )

# ios simulator - arm64
if has_ios_simulator_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_ios_profile",
                "deployment_target": "15.0",
                "sdk": "iphonesimulator",
                "group": "ios-simulator",
                "expected_platform": "IOSSIMULATOR",
                "platform": "SIMULATORARM64",
                "enable_arc": True,
                "enable_visibility": False,
            },
        ]
    )

# tvos - arm64
if has_tvos_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_tvos_profile",
                "deployment_target": "15.0",
                "sdk": "appletvos",
                "group": "tvos",
                "expected_platform": "TVOS",
                "platform": "TVOS",
                "enable_arc": True,
                "enable_visibility": False,
            },
        ]
    )

# tvos simulator - x64
if has_tvos_simulator_x64:
    data.extend(
        [
            {
                "arch": "x86_64",
                "conan_arch": "x86_64",
                "conan_profile": "xplpc_tvos_profile",
                "deployment_target": "15.0",
                "sdk": "appletvsimulator",
                "group": "tvos-simulator",
                "expected_platform": "TVOSSIMULATOR",
                "platform": "SIMULATOR_TVOS",
                "enable_arc": True,
                "enable_visibility": False,
            },
        ]
    )

# tvos simulator - arm64
if has_tvos_simulator_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_tvos_profile",
                "deployment_target": "15.0",
                "sdk": "appletvsimulator",
                "group": "tvos-simulator",
                "expected_platform": "TVOSSIMULATOR",
                "platform": "SIMULATORARM64_TVOS",
                "enable_arc": True,
                "enable_visibility": False,
            },
        ]
    )

# watchos - arm64
if has_watchos_arm64:
    data.extend(
        [
            {
                "arch": "arm64_32",
                "conan_arch": "armv8_32",
                "conan_profile": "xplpc_watchos_profile",
                "deployment_target": "9.0",
                "sdk": "watchos",
                "group": "watchos",
                "expected_platform": "WATCHOS",
                "platform": "WATCHOS",
                "enable_arc": True,
                "enable_visibility": False,
            },
        ]
    )

# watchos simulator - x64
if has_watchos_simulator_x64:
    data.extend(
        [
            {
                "arch": "x86_64",
                "conan_arch": "x86_64",
                "conan_profile": "xplpc_watchos_profile",
                "deployment_target": "9.0",
                "sdk": "watchsimulator",
                "group": "watchos-simulator",
                "expected_platform": "WATCHOSSIMULATOR",
                "platform": "SIMULATOR_WATCHOS",
                "enable_arc": True,
                "enable_visibility": False,
            },
        ]
    )

# watchos simulator - arm64
if has_watchos_simulator_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_watchos_profile",
                "deployment_target": "9.0",
                "sdk": "watchsimulator",
                "group": "watchos-simulator",
                "expected_platform": "WATCHOSSIMULATOR",
                "platform": "SIMULATORARM64_WATCHOS",
                "enable_arc": True,
                "enable_visibility": False,
            },
        ]
    )

# mac catalyst
if has_mac_catalyst_x64:
    data.extend(
        [
            {
                "arch": "x86_64",
                "conan_arch": "x86_64",
                "conan_profile": "xplpc_catalyst_profile",
                "deployment_target": "15.0",
                "group": "mac-catalyst",
                "expected_platform": "MACCATALYST",
                "subsystem_ios_version": "15.0",
                "platform": "MAC_CATALYST",
                "xcode_destination": "generic/platform=macOS,variant=Mac Catalyst",
                "enable_arc": True,
                "enable_visibility": False,
            },
        ]
    )

# mac catalyst arm64
if has_mac_catalyst_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_catalyst_profile",
                "deployment_target": "15.0",
                "group": "mac-catalyst",
                "expected_platform": "MACCATALYST",
                "subsystem_ios_version": "15.0",
                "platform": "MAC_CATALYST_ARM64",
                "xcode_destination": "generic/platform=macOS,variant=Mac Catalyst",
                "enable_arc": True,
                "enable_visibility": False,
            },
        ]
    )
