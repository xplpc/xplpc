import Foundation

/// Writes a date as whole milliseconds since the unix epoch and a slash as itself, which is how the wire carries both and what the encoder does not do by default.
enum JsonCoding {
    static func encoder() -> JSONEncoder {
        let encoder = JSONEncoder()

        // The encoder escapes a slash by default, which the wire writes as itself.
        encoder.outputFormatting = .withoutEscapingSlashes

        encoder.dateEncodingStrategy = .custom { date, encoder in
            // A date that is not finite or that whole milliseconds cannot hold would trap in the conversion, so it is refused like any value the wire cannot carry.
            guard let milliseconds = Int64(exactly: (date.timeIntervalSince1970 * 1000).rounded()) else {
                throw EncodingError.invalidValue(date, .init(codingPath: encoder.codingPath, debugDescription: "The date cannot be written as whole milliseconds"))
            }

            var container = encoder.singleValueContainer()
            try container.encode(milliseconds)
        }

        return encoder
    }
}
