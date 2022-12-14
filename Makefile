# variables
BUILD_TYPE = Release

BUILD_SWIFT_LIST = \
	swift:ios-arm64:OS64:11.0 \
	swift:ios-sim-x64:SIMULATOR64:11.0 \
	swift:ios-sim-arm64:SIMULATORARM64:11.0 \
	swift:tvos-arm64:TVOS:11.0 \
	swift:tvos-sim-x64:SIMULATOR_TVOS:11.0 \
	swift:watchos-arm64:WATCHOS:4.0 \
	swift:watchos-sim-x64:SIMULATOR_WATCHOS:4.0 \
	swift:catalyst-x64:MAC_CATALYST:13.1 \
	swift:catalyst-arm64:MAC_CATALYST_ARM64:13.1

TEST_SWIFT_LIST = \
	test-swift/tests:macos-arm64:MAC_ARM64:11.0

# exports
.EXPORT_ALL_VARIABLES:
CPM_SOURCE_CACHE = ${HOME}/.cache/CPM

# tasks
.PHONY: help
help:                  ## Show The Help
	@echo "Usage: make <target>"
	@echo ""
	@echo "Targets:"
	@fgrep "##" Makefile | fgrep -v fgrep

.PHONY: build-swift
build-swift:           ## Setup And Build For Swift
	rm -rf build/swift

	@- $(foreach IT,$(BUILD_SWIFT_LIST), \
        $(eval IT_ROOT = $(word 1,$(subst :, ,$(IT)))) \
        $(eval IT_ARCH = $(word 2,$(subst :, ,$(IT)))) \
        $(eval IT_PLAT = $(word 3,$(subst :, ,$(IT)))) \
        $(eval IT_VERS = $(word 4,$(subst :, ,$(IT)))) \
        \
		echo "Building for ${IT_ARCH}..." ; \
		rm -rf build/${IT_ROOT}/${IT_ARCH} ; \
		mkdir -p build/${IT_ROOT}/${IT_ARCH} ; \
		cmake -S . -B build/${IT_ROOT}/${IT_ARCH} -GXcode -DXPLPC_TARGET=swift -DXPLPC_ADD_CUSTOM_DATA=ON -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake -DPLATFORM=${IT_PLAT} -DDEPLOYMENT_TARGET=${IT_VERS} ; \
		cmake --build build/${IT_ROOT}/${IT_ARCH} --config ${BUILD_TYPE} ; \
    )

.PHONY: test-swift
test-swift:            ## Run Tests For Swift
	rm -rf build/test-swift

	@- $(foreach IT,$(TEST_SWIFT_LIST), \
        $(eval IT_ROOT = $(word 1,$(subst :, ,$(IT)))) \
        $(eval IT_ARCH = $(word 2,$(subst :, ,$(IT)))) \
        $(eval IT_PLAT = $(word 3,$(subst :, ,$(IT)))) \
        $(eval IT_VERS = $(word 4,$(subst :, ,$(IT)))) \
        \
		echo "Building for ${IT_ARCH}..." ; \
		rm -rf build/${IT_ROOT}/${IT_ARCH} ; \
		mkdir -p build/${IT_ROOT}/${IT_ARCH} ; \
		cmake -S . -B build/${IT_ROOT}/${IT_ARCH} -GXcode -DXPLPC_TARGET=swift -DXPLPC_ADD_CUSTOM_DATA=ON -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake -DPLATFORM=${IT_PLAT} -DDEPLOYMENT_TARGET=${IT_VERS} -DXPLPC_ENABLE_TESTS=YES ; \
		cmake --build build/${IT_ROOT}/${IT_ARCH} --config ${BUILD_TYPE} ; \
		cd build/${IT_ROOT}/${IT_ARCH} ; \
		ctest -C ${BUILD_TYPE} ; \
		cat Testing/Temporary/LastTest.log ; \
    )

.PHONY: build-xcframework
build-xcframework:     ## Build XCFramework
	rm -rf build/xcframework
	mkdir -p build/xcframework

	xcodebuild -create-xcframework \
		-framework build/swift/ios-arm64/lib/${BUILD_TYPE}/xplpc.framework \
		-framework build/swift/ios-sim-arm64/lib/${BUILD_TYPE}/xplpc.framework \
		-framework build/swift/tvos-arm64/lib/${BUILD_TYPE}/xplpc.framework \
		-framework build/swift/tvos-sim-x64/lib/${BUILD_TYPE}/xplpc.framework \
		-framework build/swift/watchos-arm64/lib/${BUILD_TYPE}/xplpc.framework \
		-framework build/swift/watchos-sim-x64/lib/${BUILD_TYPE}/xplpc.framework \
		-framework build/swift/catalyst-arm64/lib/${BUILD_TYPE}/xplpc.framework \
		-output build/xcframework/xplpc.xcframework
