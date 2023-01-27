public class UniqueID {
    private static var shared: UniqueID = .init()

    private var mutex = pthread_mutex_t()
    private var counter: UInt = 0

    private init() {
        pthread_mutex_init(&mutex, nil)
    }

    deinit {
        pthread_mutex_destroy(&mutex)
    }

    static func generate() -> String {
        pthread_mutex_lock(&shared.mutex)

        defer {
            pthread_mutex_unlock(&shared.mutex)
        }

        shared.counter += 1

        return String(shared.counter)
    }
}
