## 1.0.0

* First release of the Dart plugin.
* Calls a procedure implemented in C++, Kotlin, Swift, Python or JavaScript through `Client.call`, `Client.callSync` and `Client.callAsync`.
* Answers a call the native side makes for a mapping registered with `MappingList.add`.
* Carries a large buffer across the bridge without copying it through `DataView`.
