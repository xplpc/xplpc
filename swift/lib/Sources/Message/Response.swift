public typealias ResponseData = AnyCodable

/// A mapping is free to answer from any thread it chooses, so the responder it receives is safe to carry there.
public typealias Response = @Sendable (ResponseData?) -> Void
