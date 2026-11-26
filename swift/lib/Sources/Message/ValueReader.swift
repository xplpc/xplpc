import Foundation

/// Reads a value off the wire as the type asked for, which is the one place a parameter and an answer are read, so both refuse what the type cannot hold exactly.
enum ValueReader {
    static func read<T>(_ stored: AnyCodable) -> T? {
        exact(stored.value)
    }

    /// Reads a value as the type asked for, which is also how every number, boolean and text inside a decoded value is read.
    static func exact<T>(_ value: Any) -> T? {
        if let integerType = T.self as? any BinaryInteger.Type {
            return integer(value, as: integerType) as? T
        }

        if let floatingType = T.self as? any BinaryFloatingPoint.Type {
            return floating(value, as: floatingType) as? T
        }

        if T.self == Bool.self {
            return boolean(value) as? T
        }

        if T.self == String.self {
            return value as? T
        }

        // A cast reads a boolean inside a list or an object as a number and a number as a boolean, so a type that can be decoded is never cast from one.
        if T.self is any Decodable.Type, value is [Any] || value is [String: Any] {
            return decode(value)
        }

        return value as? T ?? decode(value)
    }

    /// A value that is not the type itself is decoded into it, which is how a date, a data view or a type of the caller's own is read.
    private static func decode<T>(_ value: Any) -> T? {
        guard let decodableType = T.self as? any Decodable.Type else {
            return nil
        }

        return (try? ValueDecoder(value).read(decodableType)) as? T
    }

    /// A number knows whether it was written as an integer, a fraction or a boolean, which a cast to a swift type forgets, so each kind is told apart before it is converted.
    private enum Kind {
        case boolean
        case integer
        case fraction
    }

    private static func kind(of number: NSNumber) -> Kind {
        // The parser hands back a decimal number only for an integer past every 64 bit type, which only a floating type can hold.
        if number is NSDecimalNumber || CFNumberIsFloatType(number) {
            return .fraction
        }

        return CFGetTypeID(number) == CFBooleanGetTypeID() ? .boolean : .integer
    }

    /// The parser keeps an integer past the signed 64 bit range as an unsigned one, which is the only case its signed value would wrap.
    private static func isUnsigned(_ number: NSNumber) -> Bool {
        String(cString: number.objCType) == "Q"
    }

    private static func boolean(_ value: Any) -> Bool? {
        guard let number = value as? NSNumber, kind(of: number) == .boolean else {
            return nil
        }

        return number.boolValue
    }

    private static func integer<I: BinaryInteger>(_ value: Any, as _: I.Type) -> I? {
        guard let number = value as? NSNumber, kind(of: number) == .integer else {
            return nil
        }

        return isUnsigned(number) ? I(exactly: number.uint64Value) : I(exactly: number.int64Value)
    }

    private static func floating<F: BinaryFloatingPoint>(_ value: Any, as _: F.Type) -> F? {
        guard let number = value as? NSNumber else {
            return nil
        }

        let numberKind = kind(of: number)

        if numberKind == .boolean {
            return nil
        }

        if numberKind == .integer {
            return isUnsigned(number) ? F(exactly: number.uint64Value) : F(exactly: number.int64Value)
        }

        // A fraction is rounded to the nearest value the type holds, and a magnitude past the type is refused rather than read as infinity.
        let wide = number.doubleValue

        guard wide.isFinite, wide.magnitude <= Double(F.greatestFiniteMagnitude) else {
            return nil
        }

        return F(wide)
    }
}
