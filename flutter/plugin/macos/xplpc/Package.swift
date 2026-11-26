// swift-tools-version: 5.9

import PackageDescription

let package = Package(
    name: "xplpc",
    platforms: [.macOS("12.0")],
    products: [.library(name: "xplpc", targets: ["xplpc"])],
    targets: [.binaryTarget(name: "xplpc", path: "../xplpc.xcframework")]
)
