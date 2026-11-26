import Foundation

/// Opens the shared conformance cases, from the repository on macos, whose path the build passes in, and from the test bundle on a simulator or a device, which carries them as a resource.
enum ConformanceFiles {
    private final class BundleMarker {}

    private static let root: URL? = {
        if let path = ProcessInfo.processInfo.environment["XPLPC_CONFORMANCE_DIR"] {
            return URL(fileURLWithPath: path, isDirectory: true)
        }

        return Bundle(for: BundleMarker.self).url(forResource: "conformance", withExtension: nil)
    }()

    static var found: Bool {
        root != nil
    }

    static func names(_ group: String) -> [String] {
        guard let root else {
            return []
        }

        let files = (try? FileManager.default.contentsOfDirectory(
            at: root.appendingPathComponent(group, isDirectory: true),
            includingPropertiesForKeys: nil
        )) ?? []

        return files
            .filter { $0.pathExtension == "json" }
            .map { $0.deletingPathExtension().lastPathComponent }
            .sorted()
    }

    static func read(_ group: String, _ name: String) -> Data? {
        guard let root else {
            return nil
        }

        return try? Data(contentsOf: root.appendingPathComponent(group).appendingPathComponent("\(name).json"))
    }
}
