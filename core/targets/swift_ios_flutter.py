# archs
has_ios_arm64 = True
has_ios_simulator_x64 = True
has_ios_simulator_arm64 = True

data = []

# ios - arm64
if has_ios_arm64:
    data.extend(
        [
            {
                "arch": "arm64",
                "conan_arch": "armv8",
                "conan_profile": "xplpc_ios_profile",
                "deployment_target": "11.0",
                "supported_platform": "iPhoneOS",
                "sdk": "iphoneos",
                "group": "ios",
                "platform": "OS64",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
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
                "deployment_target": "11.0",
                "supported_platform": "iPhoneSimulator",
                "sdk": "iphonesimulator",
                "group": "ios-simulator",
                "platform": "SIMULATOR64",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
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
                "deployment_target": "11.0",
                "supported_platform": "iPhoneSimulator",
                "sdk": "iphonesimulator",
                "group": "ios-simulator",
                "platform": "SIMULATORARM64",
                "sdk_version": "11.0",
                "enable_arc": True,
                "enable_bitcode": False,
                "enable_visibility": False,
            },
        ]
    )
