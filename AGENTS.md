# XPLPC

Cross Platform Lite Procedure Call. A C++ core lets any supported language call a procedure implemented in any other supported language, transporting serialized data through device memory instead of a network protocol.

## Architecture

One router lives for the whole process, and every language is a layer attached to it. A call always takes the same path, whichever layer starts it:

```
caller layer  ->  Router  ->  owner of the name  ->  mapping
                    |                                   |
                 CallState  <----------  answer  <-------
```

- **Router** is built on first use and never reset or destroyed. Its constructor attaches the c++ layer, and the first call or attach runs `NativeLayer::initializePlatform` once, which is where the c++ mappings are registered. It holds three things under one lock: the attached layers, the calls still waiting, and for every name the stack of layers that declared it.
- **The last registration wins.** Declaring a name puts the layer on top of that name's stack, retracting it gives the name back to the layer below, and detaching a layer retracts everything it holds. So c++ can serve `http.request`, an android layer can take it over and a flutter layer can take it from both, and when the flutter layer goes the name goes back to android, including when its isolate ends without detaching, since the plugin abandons its layer from a finalizer the isolate runs at shutdown.
- **Layer** is an interface with one method, which serves a call for a name the layer declared. `NativeLayer` serves the c++ mappings, `CLayer` a host reached through the c abi, `JNILayer` kotlin, `ObjCLayer` swift and `JavascriptLayer` the page. A layer is held by a `shared_ptr`, a dispatch copies it for as long as the invoke runs, and a completion that answers into a layer holds it too, so the destructor is the release and it runs once nothing in flight holds the layer. That is why detaching never waits and why detaching from inside the layer's own callback needs no special case.
- **A call is a handle the router allocates**, a 21 bit slot above a 32 bit generation from a `SlotTable`, so a handle whose slot was reused is refused rather than resolving the newer call, and every handle stays below two to the fifty three, where a javascript number is still exact. Nothing about a call crosses a bridge as text, and the router is the only thing that names a call.
- **CallState** resolves a call exactly once. An answer that arrives while the dispatch is still running is handed back as the value of the call, which is what `CallResult::answered` carries, and one that arrives later goes to the completion. `callSync` is therefore only the question of whether the call came back answered, and a call that did not is cancelled so its later answer resolves nothing.
- **cancel** takes the call back and answers whether it did, which decides between a cancel and an answer racing it, since whichever loses resolves nothing.
- **detach** stops the layer being routed to or calling out, answers every call it was serving and every call it made with the empty value, releases every buffer it registered, and lets the layer go.
- **ThreadScope** records on the current thread which layers started the calls running on it. A host that cannot be entered from a foreign thread is entered directly only while one of its own calls is on the stack, and receives everything else through its event loop.
- **BufferTable** holds every buffer that crosses the bridge under a handle of the same shape. Only the layer that registered a buffer releases it, a released or reused handle reads as nothing, and a size that does not fit the memory it names is refused. The jni and objective-c bridges copy the bytes while they hold the span, and the wasm one hands the page a view into the heap, which is why it reaches only the body of `withUint8Array`. The table cannot recall bytes a reader already holds, so the owner keeps them until every mapping it handed the view to is done with them, which a cancel does not end. A direct buffer registered through jni is the one kind the table keeps alive itself. A host reading through `xplpc_buffer_read` gets the address alone, so it copies the bytes before anything can release them.
- **pending** counts the calls still waiting anywhere in the process, and every bridge exposes it, which is what lets each suite hold every test to leaving none behind.
- **MappingList** holds the c++ mappings and declares each name for the native layer as it records it. Every host language keeps its own `MappingList` and declares through its layer, recording the mapping first and restoring the previous entry when declaring fails, so a call routed the moment the router hears the name already finds it.
- **Serializer** is JSON only, selected by the `XPLPC_SERIALIZER_JSON` compile definition. The wire format is `{"f": <function name>, "p": [{"n": <name>, "v": <value>}]}` for requests and `{"r": <value>}` for responses.

An incoming request is read once. `decodeRequest` answers with the function name and the parameters together, and the five host languages carry it as a `DecodedRequest` holding the name and the message.

C++ has no such type, because it decodes a parameter against the type the mapping signature declares rather than reading whatever the wire carried, so there is nothing to fill before the mapping is known. It routes from one scan all the same. `decodeFunctionName` is a sax scan, `JsonRequestScanner`, that never builds the document, the router hands the layer the name it resolved, and the executor a `MappingItem` holds receives that name rather than reading it again. The scan runs to the end of the document, since a request every language would not read the same way has to be refused before it is routed. It accepts a string `f` at the root, a `p` that is an array or null, and parameters that are objects with a name that is a string and appears once, and it refuses a key written twice in any object of the document, since one parser keeps the first value and another the last. A request built by the c++ `Request` is never scanned, so its encoder refuses a parameter named twice and a request naming no function.

Every language reimplements `Client`, `MappingList`, `Serializer`, `Message`, `Param`, `Request` and `DataView` with the same responsibilities and the same file layout, plus a `Layer` that attaches it to the router. When changing one of them, check whether the other languages need the same change.

`Response` is the exception, and there it is the shape of a mapping that differs rather than the file being absent. C++, Kotlin, Swift, Dart and Python hand a mapping something to answer through, so a mapping there takes a message and a responder and returns nothing. TypeScript hands it nothing and reads what it returns, since a mapping that answers with a promise is what a caller there already knows how to wait on.

Two pieces of per call state a host language keeps are the ones its runtime forces on it. Python keys a waiting call by the identity of the object that holds it. Dart cannot hand native code an object it would hand back, so it keeps an integer that is unique among the calls its isolate is waiting on.

The jni layer is never released once attached. It keeps global references to the kotlin `Layer` and `PendingCall` classes, which keep their class loader alive, and a runtime unloads a native library only when its class loader is collected, so the library stays loaded for the life of the process and the bridge carries no unload path. Its destructor runs only when attaching it was refused inside `JNI_OnLoad`.

`DataView` names a buffer by its handle in the `BufferTable`, letting large buffers cross the bridge without being copied or base64 encoded, and no address ever crosses the wire. The owner registers the bytes and keeps them valid and in place until it releases the handle, which each language does in its own way:

| Language | Ownership |
| --- | --- |
| C++ | `Buffer` registers bytes the caller owns for as long as it lives and its destructor releases them, and `Buffer::view` hands out the `DataView` |
| Swift | `DataView.withByteArray` registers the array for as long as its closure runs and releases it when the closure returns, so the view must not escape it and reaches only a mapping that is done with it by then |
| Kotlin | `createFromByteBuffer` registers a direct buffer and the table holds a global reference to it until `release()`, and `createFromByteArray` copies into a direct buffer first |
| Dart | `createFromByteBuffer` registers native memory the caller owns, `allocateFromByteArray` registers memory it allocated, and `dispose` releases the registration and frees what it allocated |
| TypeScript | `DataView` copies into the wasm heap and registers it, `DataView.free` releases both and ignores a view it did not allocate, and the bytes are read through `DataView.withUint8Array` |
| Python | `create_from_byte_buffer` registers the exported buffer, and the layer holds the source until `release()` |

A managed heap may move an object, so no language ever registers the address of a garbage collected array. Off heap storage or an explicit copy is used instead, and the table holds what keeps that storage alive.

Every reader goes through the table, so a view that names nothing, was released or names a slot that was reused reads as no bytes rather than as memory. A mapping that answers with a view keeps it registered until the caller has read it, because the bridges read it once the call has unwound. Only the layer that registered a buffer can release it, which the table refuses rather than leaving it to convention.

## Wire format

The C++ core defines the format and every bridge has to agree with it byte for byte, since a serializer that encodes the same value differently produces a request the other side silently misreads. The `conformance/wire` cases write those bytes down once as data and every suite encodes each of them. `docs/types.md` is the reference for every type, which says how each one travels, what each language reads it as and what it refuses, and every row of it is pinned by a case. Key order is not part of the format, because `JSONEncoder` serialises a dictionary rather than the order a `Codable` wrote its keys in, so swift asserts the same document with no whitespace, which is what it can promise.

| Value | On the wire | Notes |
| --- | --- | --- |
| Whitespace | None | The document carries no space a parser would ignore, since the bridge pays for every byte it copies |
| Text | The characters themselves | Text outside ascii is written as itself rather than escaped, and so are markup and a slash, which gson and `JSONEncoder` escape unless told not to |
| Character | Integer code point | Swift uses `CodePoint`, not `Character`, because a grapheme cluster has no single code point |
| Date | Integer milliseconds since the unix epoch | Carried as a type by C++, Kotlin and Swift only |
| DataView | `{"buffer": <handle>}` | The handle the owner registered, never an address, and below two to the fifty three so javascript reads it exactly, wherever the view sits in the document |
| Null | `null` | A parameter without a value keeps its name, and a mapping with nothing to answer travels the same way |
| No parameters | `"p": []` | An empty array, never null and never absent, and a reader treats all three as no parameters |
| Infinity and not a number | `null` or nothing | Json has no token for either, so kotlin, swift, dart and python refuse such a value with a report, while c++ and typescript write the `null` the format already carries, since their encoders do so without a walk of every answer |
| Whole number in a floating type | `2.0` or `2` | C++, Kotlin, Dart and Python write `2.0`, Swift and TypeScript write `2`, and every floating reader reads both while an integer reader refuses `2.0`, so a value that must arrive as an integer is sent from an integer type |

Dart, Python and TypeScript carry neither a character nor a date type, so a caller there sends the code point or the milliseconds itself and the serializer reports anything else rather than writing it wrong.

Swift's `JSONEncoder` writes dates as seconds since the Apple reference date by default, which is why the serializer installs its own strategy, and a date that whole milliseconds cannot hold is an encoding error rather than a trap.

A javascript number is a double, so an integer literal above `Number.MAX_SAFE_INTEGER` is rounded on parse. Every other platform reads such a value exactly, so the fix belongs on the javascript side and not on the wire. The TypeScript serializer inspects the original text through the parse reviver and rejects a value it would have to round, and a runtime that does not hand the reviver the source text gets every whole number past the safe range refused instead. Code that needs the full 64 bit range across the wasm bridge declares that field as a string in its own mapping.

## Project layout

| Path | Content |
| --- | --- |
| `cxx/lib` | C++ core library and the router, the only place where the protocol is defined |
| `conformance` | Wire, accept, refuse and values cases every suite reads |
| `cxx/custom` | Sample mappings and types, built when `XPLPC_ADD_CUSTOM_DATA` is on |
| `cxx/sample`, `cxx/tests` | C++ sample and gtest suite |
| `cxx/consumer` | A project of its own that reaches the library through `FetchContent`, the way a consumer does |
| `c/lib` | C ABI exposed as `xplpc_*` functions, used by Python and Flutter |
| `c/sample`, `c/tests` | C sample and gtest suite, which also compiles the public header as C |
| `jni/lib` | JNI bridge to Kotlin |
| `objc/lib` | Objective-C++ bridge to Swift |
| `wasm/lib` | Emscripten bindings |
| `kotlin/shared/src` | Kotlin library shared by the Android and desktop targets |
| `kotlin/shared/tests` | Kotlin suite, compiled by both targets so neither can drift from the other |
| `kotlin/android`, `kotlin/desktop` | Gradle projects for the library and the samples |
| `swift/lib/Sources`, `swift/tests` | Swift library compiled into the `xplpc` framework, and its suite |
| `swift/support` | The umbrella header and module map the framework ships |
| `swift/sample` | iOS sample, project generated from `project.yml` |
| `swift/spm` | Swift package manifest template |
| `flutter/plugin` | Dart plugin plus its example app |
| `python/lib`, `python/tests`, `python/sample` | Python package, its suite and a KivyMD sample |
| `wasm/sample` | TypeScript bridge, its suite and the Vue sample page |
| `core/`, `xplpc.py` | Build and task tooling |
| `cmake/` | The ios toolchain and dependency setup for CPM and Conan |
| `conan/` | Conan profiles, one per target platform |
| `docs/` | Per language guides and the types guide, the public documentation |
| `extras/images` | The images the README and the guides show |

## Tooling

Every task runs through `python3 xplpc.py <task>`. Run `python3 xplpc.py -h` for the full list. The common ones:

```
python3 xplpc.py cxx-test
python3 xplpc.py cxx-test --shuffle
python3 xplpc.py c-test
python3 xplpc.py swift-test
python3 xplpc.py swift-test --platform ios
python3 xplpc.py swift-test --platform ios-device
python3 xplpc.py kotlin-test --platform desktop
python3 xplpc.py kotlin-test --platform android
python3 xplpc.py flutter-test
python3 xplpc.py python-test
python3 xplpc.py wasm-test
python3 xplpc.py cxx-test-consumer
python3 xplpc.py <language>-format
python3 xplpc.py <language>-lint
```

### Building the library into a project

The library is a cmake project a consumer adds to its own rather than a tree it copies. The root creates one target, `xplpc` with the alias `xplpc::xplpc` on every target but wasm, which builds a module rather than a library a project links, and every layer adds its sources to it with `target_sources`, which is also how a consumer puts its mappings in. That is the one form that serves every target, since a shared library, the aar, the framework and the wasm module have no executable that could carry `initializePlatform`.

Configured as a subproject it leaves the parent alone. The build type is forced and the output directories are set only at the top level, for every configuration a multi configuration generator knows, the visibility presets and the standard are directory variables that stop at this directory, and what a consumer inherits by linking is what the headers need, which is C++20, the serializer definition, `SPDLOG_NO_TLS` and the spdlog and nlohmann include directories. `cxx-test-consumer` builds `cxx/consumer` against the checkout through `FETCHCONTENT_SOURCE_DIR_XPLPC` and fails unless the consumer's own mapping answered.

`NativeLayer::initializePlatform` is declared by the library and defined by whoever links it, which is where an application registers its mappings. `XPLPC_ADD_CUSTOM_DATA` is off by default, so linking the core alone without that definition is a link error. `cxx/custom`, `cxx/sample`, `c/sample` and `cxx/consumer` each provide one, and `XPLPC_CUSTOM_DATA_PATH` points a shared build at a directory of the consumer's own, which every task that builds the native library forwards from `--custom-data-path`, the c++, c and macos swift suites included, while the aar, jar and xcframework tasks package what that build produced, refusing a directory without a `CMakeLists.txt` before it clears a build.

The visibility presets are hidden, so only `XPLPC_API` and `JNIEXPORT` publish a symbol. The shared library exports the `xplpc_*` functions, and the jni library exports the nine kotlin entry points and `JNI_OnLoad`. The ndk links its c++ runtime statically, so the android build passes `--exclude-libs,ALL` to keep that runtime out of the exported table, and only the typeinfo of standard templates the code instantiates is left beside the entry points. Foundation is linked by the objective-c bridge alone, so the libraries the jvm, python and dart load on apple platforms do not bring the objective-c runtime with them.

### What a task does not rebuild

The Flutter and Python suites need the native library first:

```
python3 xplpc.py swift-build --interface --platform macos-flutter
python3 xplpc.py swift-build-xcframework --platform macos-flutter
python3 xplpc.py c-build-shared && python3 xplpc.py python-install
```

`kotlin-test` does not build the native library, so a change under `cxx`, `c` or `jni` is only picked up after `kotlin-build --platform desktop` or `--platform android`. `wasm-test` behaves the same way against `wasm-build`. Running either suite without the build tests the previous binary.

`flutter-test` deletes `flutter/plugin/build/ios/SourcePackages` and its macos twin before `pub get`, because flutter copies the plugin into its own build directory without leaving that directory out, so each run would otherwise copy the copies of the run before.

### Kotlin

Kotlin runs one suite on two runtimes. `kotlin/shared/tests` is compiled by the desktop project against the jvm and by the android project against a device, the same way `kotlin/shared/src` is, so a test cannot exist on one and not the other.

What stays per project genuinely differs. `TestPlatform` brings xplpc up, which desktop has to ask for and android already got from the content provider that is its platform initializer. `ConformanceFiles` reads the shared cases, which are files on the jvm and test assets on a device. The desktop loader tests, `NativeLibTest` with the probes it starts in a child jvm, load the library from a jar, which is how only desktop gets it.

`connectedAndroidTest` runs on every connected device, so `ANDROID_SERIAL` names the one a run is for.

### Swift

The iOS sample project is generated rather than hand edited, and building it regenerates it first so the committed project cannot drift from the spec:

```
python3 xplpc.py swift-generate-sample-project
python3 xplpc.py swift-build-sample
```

A swift package manager consumer reaches the library as a binary target, which is one archive and one checksum, so the slices cannot be split the way the other swift tasks split them:

```
python3 xplpc.py swift-build --platform apple
python3 xplpc.py swift-build-xcframework --platform apple
python3 xplpc.py swift-generate-spm-package --url=<where the archive will be published>
```

The `apple` platform exists only for this and carries ios, tvos, watchos, mac catalyst and macos at once. The archive is written with `ditto` rather than `zip`, because a macos framework is a bundle built out of symlinks. The checksum ties the package to those exact bytes, so the archive is published to the url the package names and republishing anything means regenerating both. Neither one is committed. The declared platforms are read from the same target definitions the build uses, and the package comes from `swift/spm/Package.swift.in`, with only the platform list, the url and the checksum substituted into it.

Every slice names the platform it has to come out as, and the build reads it back with `vtool` and stops when the two disagree. The assembled xcframework carries no swift source info and no abi baseline, since both name files on the machine that built it. Every framework in it carries a dSYM read from its merged binary, so the uuid a crash report names is one the dSYM answers for. The binary then loses its debug map, which names object files on that machine, and is signed ad hoc once the xcframework is assembled, since stripping drops the signature an arm64 binary needs to load. The xcode generator reads the optimization and the debug information of a target that mixes swift and c++ from the swift flags, which would build the c++ of a release at `-O1` without debug information, so the swift target names both itself. The mac catalyst slices are built through `xcodebuild -scheme` with a `generic/platform` destination, since `cmake --build` drives the xcode generator with `-target`, which drops the destination.

The swift suite runs from the sample on the simulator. An XCTest bundle needs a host application there and the sample is one, so `swift-test --platform ios` runs the same tests against the shipped xcframework rather than against the sources, which is why the suite imports `xplpc` plainly instead of testably. The first available iPhone is used. A connected iphone runs the suite through `swift-test --platform ios-device`, which is local only, since no hosted runner has one.

### Samples

The samples are the only consumers of the public surface, so the workflows build every one they can and run the console ones. The flutter example is analyzed inside `flutter-test`. The python sample is a KivyMD application that needs a display to start, so the python public surface is held by the suite, and `python-pyinstaller` packages the sample with the kivy metrics variables that let it build without a window.

### Sanitizers

The C++ and C suites also build under instrumentation:

```
python3 xplpc.py cxx-test --sanitizer thread
python3 xplpc.py cxx-test --sanitizer address
python3 xplpc.py cxx-test --sanitizer undefined
python3 xplpc.py swift-test --sanitizer undefined
python3 xplpc.py swift-build --platform ios-simulator --sanitizer thread
python3 xplpc.py swift-build-xcframework --platform ios-simulator
python3 xplpc.py swift-test --platform ios --sanitizer thread
```

`thread` finds unsynchronized access between threads, `address` finds use after free and out of bounds, and `undefined` finds signed overflow, misaligned access and invalid casts. A sanitizer only reports what the run reaches, so the stress suites make the router collide on purpose.

| Bridge | address | undefined | thread | How |
| --- | --- | --- | --- | --- |
| C++ core | yes | yes | yes | its own suite |
| C abi | yes | yes | yes | its own suite, and again with the python suite driving an instrumented shared library |
| JNI | partly | yes | no | the library is built with the sanitizer and the kotlin suite runs against it, except the thread one, which cannot be preloaded into a jvm, while the address one aborts on the first c++ exception thrown and so covers only the paths that do not raise |
| Objective-C | yes | yes | yes | undefined on macos, thread and address on the simulator |
| Dart | yes | yes | no | the flutter suite on linux against an instrumented shared library, except the thread one, which ends the dart vm through a preload |
| WASM | yes | yes | n/a | emscripten builds the module with the sanitizer, and the module is single threaded |
| Python | yes | yes | yes | the suite against an instrumented shared library, plus a run on a free threaded interpreter |

The jni, dart, python, wasm and swift simulator jobs fail before their first test unless they find the runtime linked, and the flutter job checks the plain build is plain. The c++ and c jobs build the instrumented binary themselves through `--sanitizer`. The c++ and c suites run once more under the address sanitizer with `XPLPC_LOG_LEVEL=debug`, which is the only run that executes every debug line.

On macOS the swift tests are an XCTest bundle opened with `dlopen`, and address and thread need their runtime in the process before it starts, so only undefined runs there and asking for either of the other two says so. On the simulator xcodebuild instruments the host application, so thread and address run there, against a framework built from the simulator slices alone, since `-fsanitize=thread` is an unsupported option for a device triple.

Linux finds a leak through the address sanitizer. macOS has no leak detector in that sanitizer, so `cxx-build-leaks` and `c-build-leaks` run a debug build under Apple's `leaks` with `MallocStackLogging` set.

The thread sanitizers of the two platforms reach different interleavings, so anything that answers from a thread is verified by the linux result and not only the local one.

### Order

`cxx-test --shuffle` and `c-test --shuffle` run the test binary whole, in a random order and twice, since ctest runs each test in a process of its own and leaves nothing to shuffle. The python job runs the suite with `-p randomly`, which `python/tests/pytest.ini` turns off for the ordinary run. The dart and typescript files that bring the library up run their tests before the initialize first, since neither runtime can take the library down again, and both runners keep source order.

### Dependencies

Dependencies come from CPM by default, or Conan with `--dependency-tool conan`. Both resolve the same versions, declared in `cmake/cpm/cpm-dependency.cmake` and `conanfile.py`, and must be kept in sync. The c++ and c suites run through conan on the linux, macos and both windows jobs.

Spdlog is built with `SPDLOG_NO_TLS` on both paths. Cpm passes it as an option, and conan passes it as an extra cmake variable of the spdlog package with that configuration made part of its package id, so a binary built without it is never reused, while the definition reaches consumers through the `xplpc` target since the recipe does not hand it on.

The Conan path targets Conan 2. The profile is created with `conan profile detect`, the Apple toolchain switches are configuration entries passed with `-c:h tools.apple:*`, `conan install` writes into `--output-folder`, and Mac catalyst needs `os.subsystem.ios_version` rather than an `os.sdk`. What a profile names for arch, `os.version` and `os.sdk` is a default the task overrides from the target definitions, and `conan-setup` resolves all nine profiles against the installed conan in the main job of every language workflow.

Every dependency names an exact version or a checksum. CPM names a hash for its bootstrap, the gradle wrappers name the checksum of their distribution, every action names a release tag, conan and the python requirements name exact versions, and the javascript lock is committed and installed with `npm ci`. An application commits its lock and a library does not. What cannot be pinned is the tooling a runner installs for itself: swiftlint, detekt and xcodegen come from brew, which cannot pin a formula, the workflows upgrade pip before installing the pinned requirements, the linux jobs take apt packages from the image, and an action names a major release tag rather than an exact one. Black, prettier, remark and dart format are pinned and run in a job that fails on any diff, while clang-format, swiftformat and ktlint come from the system or from brew and are run locally.

### Platforms

Every cpm and conan configure of a target on macos passes the deployment target its target definition declares, which is 12.0, so the shared library, the jar's library and the wheel's library are built for that floor rather than for the machine that built them. The consumer project keeps the default of its own, the way a project that fetches the library does.

The android modules name `ndkVersion` as the ndk the native build uses, so the release apk is stripped by that ndk, and a step reads the library back out of the release apk and fails unless it is stripped. The aar keeps its symbols, since the application extracts them with `debugSymbolLevel` and strips while it packages. The release sample is minified, and a step reads r8's `seeds.txt` back to check the six fields the wire format reads by name and the two methods jni resolves are kept. Android lint runs on both modules with `warningsAsErrors`.

The python wheel is tagged with the platform its library was built for and ships the license and the `py.typed` marker, and no source distribution is built, since nothing can build the native library from one. A wheel is never tagged below the floor of the interpreter that built it.

The flutter plugin points at the local build for ios and macos through a link `swift-build-xcframework` creates, and carries a swift package manager manifest beside its podspecs.

The wasm module is linked with `-sASSERTIONS` only in a debug build, so the sanitizer job reads the runtime it linked out of the `.wasm` binary rather than out of the script. The wasm sample is built for the path it is published under and served under that path, and its workflow publishes the demo page to a second repository through a deploy key.

Every workflow carries `workflow_dispatch`, and each one names a concurrency group per ref that cancels what it supersedes except on the default branch.

## Testing

Every suite drives the same round trip, and a green run proves nothing unless it proves the callback ran. Each bridge loses a failing assertion inside a callback differently: jni clears a pending java exception, a dart `NativeCallable` cannot let an exception cross the native frame, and a typescript test that does not return its promise finishes first. So each suite verifies invocation and not only the value:

| Bridge | Guard |
| --- | --- |
| C++ | `VerifiedCall::run` for an inline answer, `AnswerWaiter` when the mapping answers later |
| C | `TestHost::waitForPosts` and `TestHost::invocations` |
| Swift | `verifyCall`, built on `XCTestExpectation` |
| Kotlin | `verifyCall`, which captures what the callback threw and raises it after the call |
| Dart | `verifyCall` and `verifyCallFromString`, the same shape |
| TypeScript | the test returns the promise, so the runner waits for it |
| Python | `verify_call` and `verify_call_from_string`, which wait for the answer and assert it arrived |

Every fixture asserts after every test that the router holds no pending call, which is `Router::shared().pending()` on c++ and c, `Layer.pending()` on kotlin, `ObjCLayerBridge.pending()` on swift, `Layer.pending` on dart, `Layer().pending_count()` on python and `JavascriptBridge.pendingCount()` on typescript. Dart and python also hold every test to releasing every buffer it registered, and their counts add the tokens each keeps for its own waiting callers.

The rules every decoder has to read alike are written once as data under `conformance/`, 216 cases in four groups, which are wire, accept, refuse and values. A runner in every suite but the c abi one, which has no serializer of its own, reads every file of the groups that apply to it when the suite runs and reports each case by its file name, and on c++ ctest discovers the cases when it runs, so a case added to an incremental build is not skipped. Values runs on c++, kotlin, swift, dart and python, the bridges that read an answer as a declared type. A runner that meets a type its language has no equivalent for skips the case by name with the reason, and one that meets a type it does not know fails it, so a misspelled type is never passed over. Routing, calls and buffers have no group, because the router and the buffer table implement them once in the core, so they are tested there and each bridge tests only what its own layer adds.

The values a type test carries are the edges, which are the smallest and largest value of every signed and unsigned width, one past each, a fraction, an integral fraction, a boolean, text and null, an integer past what a double holds exactly, a fraction no binary float represents, and for every list, map and record a value inside it that breaks a rule. The wasm bridge sends `Number.MAX_SAFE_INTEGER` instead of the integer past a double, since a javascript number cannot hold it even as a literal. Only the shape a bridge actually returns is asserted on: c++, kotlin and swift decode a list of objects into instances, python answers null, and dart and typescript answer the decoded maps.

A call has three shapes and each is checked on every bridge: a caller reaching a mapping the core owns, the core reaching back for a mapping the caller owns, which `sample.reverse` does by calling `platform.reverse.response`, and the native side invoking a host mapping directly, which is `platform.battery.level`. The nested shape is also checked with the inner mapping answering later, and the c suite reaches all of them through the abi, including a call for a host owned name from a thread the host did not start, which travels through `post`.

A test that needs a mapping to answer later holds the responder and answers it itself once it has asserted, rather than letting a thread answer after a sleep, so a loaded machine cannot reorder what the test asserts. The c `TestHost` serving later waits for `releaseLater`. A test waits on an event and never asserts an elapsed time.

A guard is worth only what it was proven to be worth. Every guard is proven by removing it and watching its test fail, and a test whose assertion survives the removal of the behaviour it names is rewritten. A mutation harness reads the exit code, carries a timeout, and treats a mutation that does not build as its own outcome rather than as a catch.

A test name says which side does what, since reading the names is how coverage is judged. A stress suite clears its registries before each test as well as after.

Two shapes are not expressible everywhere. A call made from a thread the bridge does not own means nothing on dart or javascript, where an isolate and a page each run on one. A mapping that answers later cannot run on wasm, which is linked without threads, and the suite pins that it is answered with the empty value.

## What every change keeps

### Callers and answers

- No caller is left waiting, on any bridge, including when the host cannot be entered from the current thread, when the library is not initialized, when the layer detached and when a mapping, a serializer or a callback raises.
- A call is answered exactly once, whichever of an answer, a cancel and a detach arrives first.
- A call made before the library is up says so by name on every host bridge. Swift, dart and typescript write one line and answer with the empty value, python writes that line and raises, and kotlin raises, from the `lateinit` config for a typed call and as an unsatisfied link for a call from a document made before the native library loaded.
- A call is cancelled only by the layer that made it.
- A mapping never holds the bridge. It starts its work and returns, and the library creates no thread.
- A host records a mapping before declaring its name and restores the previous entry when declaring fails.
- A name a host declared without a mapping behind it is answered with the empty value.
- A cancelled call is taken back out of the router on the bridges where a waiting caller can be abandoned, which are c++, kotlin, swift and python.

### Boundaries

- A c++ exception never leaves an `extern "C"` function, a jni entry point or an objective-c entry point, since every one runs through `NativeBoundary` and every completion through `CallState::run`.
- A kotlin catch around user code catches `Throwable`, since an `Error` such as a stack overflow is not an `Exception`.
- An exception never crosses a ctypes callback or a dart `NativeCallable`, since a python or dart callback that raises is caught and reported where the answer is delivered, and a python mapping that raises anything, `SystemExit` and `KeyboardInterrupt` included, answers its caller.
- Encoding an answer or a request never raises out of a serializer, and a value the wire cannot carry is refused with a report and answered with the empty value, except a number that is not finite, which c++ and typescript write as `null`.
- A serializer of the caller's own that raises while it writes a request or reads an answer is reported by the client, and the caller is answered with the empty value.
- Bytes handed over by native code are decoded with a guard, since the c abi is exported and the bytes are not this library's to trust. A c host can answer any caller with any bytes, so swift, dart and python report an answer that is not utf8, and kotlin carries a malformed sequence as the replacement character.

### Logs

- An error line names what failed and where, which is a function, a parameter, a class or an entry point, and never carries a request, a response, a value or exception text. The reason goes on a following debug line.
- One event writes one error line. The decoder reports a request it cannot read and the layer answers without repeating it.
- The empty response is the documented answer of every failing path and is never reported as a decode error, and a response that carries no value is answered as nothing without a report.
- Every bridge writes through a named logger, `XPLPC`, and never configures the process wide logger or a host's default one.

### Values

- A number is read into a numeric type only when that type holds it exactly, on c++, kotlin, swift and dart, whether it sits at the top of a value or inside a list, a map or a consumer type, and a boolean and a number never read as each other. A floating type rounds a fraction to the nearest value it holds, needs an integer to arrive exactly and refuses a magnitude past its range.
- A number past what a double holds and the tokens `NaN` and `Infinity` are refused by every decoder, and python never builds a built in type out of an object.
- A c++ `char` is read only from code points 0 through 127, since its signedness differs between abis.
- A timestamp past what the declared clock counts is refused.
- A parameter with a name and no value reads as null, and a parameter with no name is refused.
- A data view is read only from an unsigned integer handle that fits, and a boolean is never a handle. A view is written wherever it sits in a value, not only as a parameter of its own, and a value that is not a view is the empty view wherever it sits.
- A nested value the declared type cannot hold, such as a date or a character inside a consumer type, refuses the whole value rather than leaving a field the type says cannot be empty without a value.
- A field a type declares as never empty refuses the whole value when the wire carries null for it, on c++, kotlin and swift, and a null inside a list, a map or a type argument that declares none refuses it too. A field the wire leaves out takes the default the type declares, and one that has none refuses the value.
- A consumer type in the samples reads each field through `JsonField`, which applies the same check and refuses a value that is not an object, and a nested list is bounded in depth.

### Buffers and sizes

- A buffer is released only by the layer that registered it, and a size that does not fit the memory it names is refused.
- A jni read of a buffer or a text longer than a java array or string can index is refused rather than cut.
- A mapping never indexes a buffer by dimensions the caller declared without checking them against the size it received, and a stride walk stops at the last whole element.

### Lifecycle

- `initialize` publishes the initialized flag last, and a second `initialize` changes nothing.
- A process wide object a mapping can reach from a thread of its own is built once and never destroyed, which is the router, the buffer table, the mapping list and the logger.
- A dart bridge belongs to the isolate that attached it. Another isolate attaching under the same name replaces it, and an isolate that ends abandons its layer through a `NativeFinalizer`.
- `xplpc_abandon` and attaching under a taken name both wait for anything still inside the host they replace, so a host never calls either from inside one of its own callbacks.
- Python detaches its layer from an `atexit` hook while the interpreter can still receive what that sends.

### Platforms

- Nothing keeps per thread state with a destructor except the jni attachment, which runs only on bionic and the jvm, since mingw frees thread storage before it destroys what that storage holds.
- A public header compiles after `<windows.h>`, so `std::numeric_limits<T>::max()` and `min()` are written in parentheses.
- A public c header compiles as c, which `ThePublicHeaderIsValidC` checks.

## Before a change is finished

- The fix is proven by a test that fails with the fix reverted, or the change is a deletion every suite confirms.
- The same question is asked of all six bridges, since they share one design, and each answer is the same fix or a written reason why that language cannot have the defect.
- Every suite of a touched language passes, the c and c++ ones also shuffled and under the sanitizers that touch them, including on the systems the change affects that the local machine is not.
- Every formatter changes nothing and every linter reports nothing.
- A task runner function that is renamed or moved is searched for in `.github` as well, since a workflow step imports it directly.
- A change to what an artifact contains is checked against every workflow step that reads that artifact, since a step that greps a build output is a test of it.
- This file and the guides say what the code now does.

## Known limits

- Swiftlint, detekt and xcodegen come from brew, which cannot pin a formula, so a release that adds a rule or changes a generated project can turn a push red with nothing in the repository having changed.
- The six conan profiles for the cross targets resolve and build their dependencies on a mac but no job runs them, since that would compile fmt and spdlog from source six more times on every push.
- Conan center carries android-ndk r29 and emsdk 3.1.73 at newest, so the android and wasm profiles build the dependencies with older tools than the cpm path, which uses the ndk 30.0.16248370 the modules name and emsdk 7.0.1.
- Building every apple slice, the xcframework and the swift package together runs at release rather than in a job.
- The python sample needs a display, so no hosted runner starts it.
- Python has one integer type, so it reads and writes an integer of any size, and an answer carrying one past what a double holds is a document the other bridges refuse as unreadable.
- Dart reads an integer literal past what an int holds as a rounded double before the bridge sees it, the way javascript does, so a dart caller reading such a value as a double gets the rounded one, which the other typed bridges refuse.
- A shared build made with mingw is named `libxplpc.dll` and links the mingw runtime dynamically, while python and dart load the msvc build, which is the one this project produces for them.
- A dart isolate attaching under the name another one holds takes the bridge from it whether or not it still runs, since an isolate that restarts in the same process attaches under its name again, so a live isolate replaced that way, such as a second one started with `compute` that initializes the plugin, stops being answered and its waiting calls are dropped.
- The jni attachment keeps per thread state with a destructor, which is safe on the runtimes this project builds jni for, bionic and the jvm with msvc on windows, and is not on a jni build made with mingw.
- An answer is not scanned the way a request is, so one a c host writes with a key twice reads as the last value on every bridge but swift, whose parser keeps the first.
- A java class read by kotlin carries no nullability, so gson fills it with whatever the wire carried, null included.
- Python fills the fields of a record with whatever the wire carried, since an annotation there is not a type the language checks.
- Typescript erases its types when it compiles, so a generic read answers whatever the wire carried, a field typed as never null included.

## Decisions that are settled

No workflow publishes a package. The workflows verify and none of them ships a library, because this repository is a template someone clones rather than a product installed from a registry. The artefacts are built and validated so a clone can produce them, and pushing them anywhere belongs to whoever cloned it. The one thing a workflow publishes is the wasm demo page, which is documentation showing the bridge running in a browser.

Every manifest declares `1.0.0` and stays there. Versioning is a release cut from a commit, named `main-` followed by its short hash, rather than a number moved through the manifests, so a consumer pins a `main-<commit>` release.

The flutter plugin points at the local build for ios and macos. Three of its five platforms build from source through cmake and those two vendor an xcframework, which is what lets the sample run against what was just built instead of against something published.

## Code standards

These apply to the library, the bridges, the samples, the suites and the build files, in every language. Three things are outside them: the task runner under `core/` and `xplpc.py` is not shipped code and keeps its own conventions, `cmake/ios/ios.toolchain.cmake` is vendored under a licence whose header and documentation are not ours to rewrite, and the flutter example's linux, windows, macos, ios and android directories are what `flutter create` writes. Each language has a format task reading its own configuration, which is `.clang-format`, `.editorconfig`, `.swiftformat`, `black`, prettier and remark, and `swift-lint`, `kotlin-lint` and `python-lint` run the linters that read `.swiftlint.yml`, `detekt.yml`, `ruff.toml` and `mypy.ini`.

**Comments.** Rare and only where the code cannot explain itself. Every comment is a complete sentence, starts with a capital letter and ends with a full stop, and a second sentence starts on its own line rather than continuing the first. A sentence is never split across lines. When a sentence would have to begin with an identifier spelled in lowercase, the identifier keeps its exact spelling and the sentence is rewritten so it does not sit at the start. A comment above a function, method, class or module says what it does for whoever calls it and never how it is implemented inside. No decorative separators, no section banners, including in build files, no restating what the next line does, no history, and nothing verbose, fragmented or narrative. A semicolon never splits one sentence into two, which applies to this file as much as to the code. Code and comments are in English.

**Structure.** A function has a visible beginning, middle and end. Separate blocks of different responsibility with one blank line, and never stack conditionals, loops, state changes and returns into one visual block. Prefer early returns, never write `else` after `return`, and avoid nesting that an early return removes. Extract a function when one accumulates responsibilities, not to make it shorter.

**No legacy.** No backward compatibility shims, no version checks for how something used to work, no generic fallbacks, no dead code. When behaviour changes, change every call site.

**Errors.** Failing paths log through the platform logger and answer the caller with the empty value for its type. A call for a host that cannot be entered from the current thread travels through its `post` callback, a layer whose host is gone answers, a host layer reached before the library was initialized answers, and a layer that detaches answers every call it was serving and every call it made.

**Entry points.** The public surface is small and fixed: `XPLPC.initialize` on the five host bridges, `MappingList.add`, `Client.call`, `Client.callSync` and `Client.callAsync`, `DataView`, and the `xplpc_*` C functions. C++ has no initialize, since the router comes up on first use, and it adds `Buffer`, which registers bytes for as long as it lives, and `CallAwaitable`, which is parameterised on what the call answers with, so the typed form answers `std::optional<T>` and the one taking a document answers `std::string`. Every call entry point has a typed form and a form that takes the request already serialized. Internals are refactored freely, entry point signatures are not. Only `XPLPC_API` and `JNIEXPORT` publish a symbol, so a new entry point that is not marked is not reachable and a helper that is marked widens the surface. A helper added to a bridge is named the way that language spells internal.

**Types.** A value read out of a `Message` is checked against the requested type and answered with null when it does not match, on the four bridges where reading one names a type, and the mismatch is reported through the logger. Python names no type, since `Message.get` answers whatever the wire carried, and TypeScript names one it cannot use, because its generics are erased. A number is read into a numeric type only when that type holds it exactly, as the values rules above state. The cast stays direct rather than taking a guard from the caller, so a mismatch surfaces where the value is used.

**Initialization.** `initialize` flips the initialized flag last, and on kotlin and swift it publishes the config together with the flag once the native side came up. A failure in between never leaves a singleton that reports itself as ready, and the flag is its own state rather than derived from the config. C++ has no flag, since `initializePlatform` runs under a `call_once` before the first attach or call is routed.

**What a log may carry.** A failing path reports the failure through `Log.e` and carries the detail on a following `Log.d`, which is compiled out or switched off in a release build, since the text a parser produces echoes the input it choked on.

**Debug logging.** Each runtime decides for itself whether `Log.d` reaches anything. C++ reads `XPLPC_LOG_LEVEL`, and on android the `debug.xplpc.level` system property, since an application there cannot set its own environment. Swift writes to an `os_log` subsystem the system raises with `log config`. TypeScript keeps it for a development build, since the consumer compiles the source into their own bundle. Kotlin gates it on `Log.isEnabled`, which the android initializer sets from the application's debuggable flag and the desktop one from `XPLPC_LOG_LEVEL`. Dart and Python leave the decision to the level their logging framework is configured with. `Log.e` always reports.

**Answering without an event loop.** `call` takes a callback and needs no loop, which is what serves a caller that has none.

**Single threaded runtimes.** Dart isolates and JavaScript have no preemption between synchronous statements, so registries there are plain synchronous maps. A lock adds nothing and turns registration into a microtask, which loses the answer when the native side replies in the same tick. The wasm module is linked without pthreads, so a mapping that starts a thread cannot run there and is answered with the empty value.

**What a call promises.** `call` hands the request over and answers through a callback. `callAsync` hands it over and answers through the waiting form of the language, and on the five host bridges always gives control back to the caller's scheduler before the answer arrives, while the c++ awaitable continues without suspending when the answer is already there. `callSync` hands it over and answers with the value itself, and only when the mapping had already answered by the time the dispatch returned. TypeScript has a single `call` that answers with a promise, since a promise is both shapes at once.

`callSync` never waits, which is what makes it expressible on all six, since dart and typescript have one thread apiece. A mapping that had not answered has its call cancelled, so its later answer resolves nothing, and the caller is told through the log rather than through the value. A TypeScript mapping answers with a promise, which resolves in a microtask and never inside the call, so `callSync` there reaches only the mappings the wasm module owns.

Kotlin runs straight through `suspendCancellableCoroutine` when the continuation is resumed inside the block, so `callAsync` yields before dispatching. The other bridges get the suspension from their own machinery. C++ has no scheduler, so `callAsync` there is an awaitable that suspends whenever the answer is not already there and resumes on the thread that produced it. An answer that arrives while the coroutine is still suspending is taken by the return value of `await_suspend` rather than by resuming, since resuming there would destroy the frame that function is about to return to.

The callback of a call that took time runs on the thread that produced the answer, not on the thread that made the call, so anything it touches has to be safe there, and it captures owned copies since the calling frame is long gone.

**Answering from another thread.** Which way an answer travels is decided by the thread it is produced on and by the host it goes to, never by who made the call. Only a host reached through the C ABI can be out of reach from a thread, and Dart and Python are the two hosts reached that way. JNI attaches a native thread to the JVM through `ScopedThreadAttachment`, an Objective-C object takes a message on any thread, and the WASM module only has the one thread JavaScript gave it.

A c host passes two callbacks. `invoke` is entered synchronously with borrowed bytes, and `post` carries everything else in the order it happened, which is a late answer, a call arriving on a thread the host cannot be entered on, and last the release, with bytes the host owns and frees with `xplpc_free`. Dart is entered through `invoke` only on its own thread and receives everything else through a `NativeCallable.listener` that queues it onto the event loop. Python can be entered from any thread, so it says so when it attaches and `post` carries only its late answers.

A call made from a user interface with a network request on the other side puts all of this together, and nothing in it blocks the interface:

```
ui thread          Client.call hands the request over and returns immediately
native             the mapping starts the request, leaves it running and returns
ui thread          free, the whole crossing took microseconds
network            the request completes, later, on a thread of its own
that thread        the mapping calls r(value)
c layer            ThreadScope holds no call of the isolate here, so post carries the answer
isolate            the listener queues the answer onto the event loop
ui thread          the callback runs and the interface updates
```

When the mapping doing the request belongs to the host, the call reaches it on the host thread and it returns immediately with the request in flight, and the answer it produces later goes back through `xplpc_answer` on whatever thread the host chose. The router never asks a host whether it owns a function, since every layer declares its names and a call is routed from what was declared.

### C++

- C++20, four space indent, Allman braces, `#pragma once` in headers.
- Namespaces are `xplpc::<area>`, closed with `} // namespace <name>`.
- No anonymous namespaces. Something either belongs to a class, or it belongs to a helper class, one class per file.
- Prefer `const`, references and smart pointers. No raw owning pointers, no C casts, no macros where a constant or template works.
- A comment on a declaration says what it does for whoever calls it, and nothing in a header describes a section.
- Shared state is guarded. The router, the buffer table and the mapping list each hold a `std::mutex` over their state, `TSMap` guards a keyed collection with a shared mutex of its own, `std::atomic` holds flags and published handles, and `std::call_once` runs `initializePlatform` once.
- Iterating a shared container while calling out to user code is done over a snapshot taken under the lock, so a callback can reenter without deadlocking.
- A failure is decided under the lock and reported once it is free, since a sink of the logger is code of the host's own and may call back into the library.
- Lambdas do not format well, so wrap them in `// clang-format off` and `// clang-format on` and hand format the body.
- Callbacks that outlive the current frame capture owned copies, never references or borrowed pointers.

### C interface

- Public headers are valid C. Types come from `<stdbool.h>`, `<stddef.h>` and `<stdint.h>`, and the declarations sit inside an `extern "C"` block guarded by `__cplusplus`.
- Exported symbols are `xplpc_*` and marked with `XPLPC_API`.
- Every entry point refuses a null pointer it would read, except that `xplpc_free` accepts null and `xplpc_answer` given a null response answers the empty value, and every one accepts only a layer handle this abi handed out.
- Buffers arrive as pointer plus size, always. Copy them before the call can outlive the frame, and never rely on null termination.
- A host declares the names it owns with `xplpc_declare` and gives them back with `xplpc_retract`.
- A buffer crosses as the handle `xplpc_buffer_register` answers, and the host keeps the bytes valid and in place until it releases that handle.
- What `post` carries is owned by the host and released with `xplpc_free`. What `invoke` carries is borrowed for the length of the call.
- A host answers a call that arrives through `post` as `XPLPC_POST_INVOKE` the same way it answers one through `invoke`.
- The release is posted only to a host whose attach succeeded, so a host that got zero still owns its context.

### JNI

- Local references are wrapped in `ScopedLocalRef`, never left for the frame to reclaim.
- Classes and method ids are resolved once in `JNI_OnLoad` and kept by `JNILayer` as global references.
- Strings cross through `JNISupport`, which converts UTF-16 to real UTF-8 rather than the modified UTF-8 that `GetStringUTFChars` returns, and carries a malformed sequence as the replacement character.
- Every call into Java is followed by an exception check that reports and clears, since a pending exception makes the next JNI call undefined. A failed class or method lookup leaves an exception pending too, so each lookup is checked and cleared before the next one is made.
- The address of a `jbyteArray` is never taken. A direct `ByteBuffer` is registered whenever native code needs a stable address.
- Callbacks that escape a JNI entry point convert every `jstring` to `std::string` first.

### Objective-C++

- Bridge classes live in `.mm` files and expose C++ types only through the `xplpc::layer` namespace.
- The layer holds the swift host weakly and promotes it on every invoke, so a late invoke is answered with the empty value instead of reaching a deallocated object.
- Never name a class method `initialize`, it collides with the runtime hook.
- The two bridge headers the framework ships are guarded with `#ifndef`, since anything including them may be plain C, and the umbrella header beside them includes them with `#import`. Everything else here uses `#pragma once` like the rest of the project.

### Swift

- Types are `public` only when the sample or the tests need them, and a type carrying no public member is not public at all.
- Shared state is guarded by `NSLock` with `defer { unlock() }`, never by a semaphore, and the lock is released on every path including the miss path.
- Pointers into Swift storage are only valid inside the closure that produced them, so any API handing one out is scoped, like `DataView.withByteArray`.
- A continuation is resumed exactly once, whichever of the answer and a cancel arrives first.
- A type with no mutable state is checked `Sendable`, and only a type a lock guards carries `@unchecked Sendable`. The library compiles in the swift 6 language mode.
- Wherever an `AnyCodable` is the other branch of an expression, an absent value is written `Optional.none`, since a bare `nil` there becomes that type's nil literal and stores a value.
- `Log` writes to an `os_log` subsystem named `XPLPC`, raised to debug with `sudo log config --subsystem XPLPC --mode "level:debug"`. An error line is public and a debug line private, since `os_log` redacts a dynamic string outside a debugger.

### Kotlin

- The library lives in `kotlin/shared/src` and is compiled by both the Android and the desktop Gradle projects, so it must not depend on the Android SDK.
- Shared collections are `ConcurrentHashMap`. A call has no kotlin registry at all, since the router holds it and the native side answers the `PendingCall` handed over with it.
- A kotlin class is filled through its primary constructor with kotlin reflection, which reads what the jvm erases, so a null or missing field the class declares as never null refuses the value, a constructor default fills a missing field and every `init` block runs. `Client.decode` and `Message.get` check what they read against `typeOf` of the requested type, which is how a null inside a list, a map or a type argument is refused. A java class carries no nullability and is filled by gson's own reflection. The factory is registered first, since gson asks the adapter registered last first, so every type with a reader of its own keeps it.
- Gson is built once and reused, it is immutable and thread safe after `create()`, and it reads strictly. A number of no declared type, inside an `Any`, a map or a `Number` field, is read as a `Long` when it is an integer that fits one and as a `Double` that holds the literal otherwise, and refused past both.
- Coroutine continuations check `isActive` before resuming, and the call is made before `invokeOnCancellation` is installed, which cancels it through the router.
- Serialization is reflective, since any consumer type must cross the bridge without annotations or a generator. That makes `consumer-rules.pro` part of the contract: the classes JNI resolves by name, the fields the serializer reads to build the wire format, the handle a `DataView` carries and the `kotlin.Metadata` annotation kotlin reflection reads are kept there. A field added to `Param` or a DTO without a matching keep rule breaks release builds only.
- A consumer type the serializer fills reflectively needs a keep rule of its own in the application, which keeps its fields and its constructor and which the kotlin guide shows.

### Dart

- An isolate is single threaded, so registries are plain synchronous maps. Never make registration async, the native side can answer before the future completes.
- An ffi signature declares the width the C side actually uses.
- The isolate is entered synchronously only through a `NativeCallable.isolateLocal` on its own thread, and everything that may arrive from another thread comes through a `NativeCallable.listener`. Both have `keepIsolateAlive` false and live until the isolate ends.
- Text crosses to native code as the bytes `utf8.encode` produced with their count, never measured with `strlen`, and every allocation is freed in a `finally` inside `Layer`.
- `Layer.call` hands an inline answer back inside the `PendingCall` and registers a token only for a call that is still waiting, so the client runs the caller's callback outside the guard around the native call.
- Native memory owned by a Dart object is released through an explicit `dispose`, and the factory that allocates it says so.
- The suite runs its files with `concurrency: 1`, since every isolate attaches under the same name and overlapping files would take the bridge from each other.
- Reflectable output is generated, run `dart run build_runner build` after changing a reflected type.

### TypeScript and WASM

- The sample is a Vue 3 application built by Vite, linted by ESLint flat config in `eslint.config.js` and formatted by Prettier through `prettier/prettier`.
- TypeScript stays on 6 until the tools around it support 7, since `vue-tsc` and `typescript-eslint` both reach into the javascript entry points the native compiler dropped.
- No `@ts-ignore` and no unsafe `as`. The Emscripten surface is declared in `WasmModule`, including the runtime exports enabled by `EXPORTED_FUNCTIONS` and `EXPORTED_RUNTIME_METHODS`. Where the runtime offers something the standard library does not describe yet, such as the original text a reviver receives, it is declared rather than cast away.
- The classes carry the same names as every other bridge, and a file that needs both a global and the bridge class of one name renames one at the import.
- Values coming from the DOM are narrowed with `instanceof`, never cast. A template ref that is not mounted yet returns null and the caller returns early.
- Heap memory allocated through `DataView` is owned by the returned view and released with `DataView.free`.
- The heap grows into a new buffer, which detaches every typed array taken from the old one, so the bytes are never handed out. `DataView.withUint8Array` and `withUint8ClampedArray` build the array from the current buffer and pass it to the body.
- Attaching, declaring and calling are synchronous. The wasm module answers inside the same tick, so an awaited step would be queued after the answer.
- A handle is a `BigInt` on the javascript side and a number on the wire, which stays exact since the router keeps every handle below two to the fifty three.
- A generic read returns `T | undefined`, and the type parameter cannot be verified at runtime, so a caller reading a value the wire did not carry gets what the wire did carry.
- Deprecation warnings are only silenced for third party stylesheets, never for project code.

### Python

- Package layout is `src/xplpc/<area>/<module>.py`, built from `pyproject.toml`.
- Buffers cross to C as `POINTER(c_char)` with an explicit length, encoded once. `c_char_p` would stop at the first null byte and silently truncate the payload.
- The ctypes callback objects are held by the layer for as long as the library is loaded, since letting them be collected leaves the native side holding a dangling function pointer.
- Singletons are built completely before they are published, guard construction with a class level lock and their data with a separate instance lock.
- The package opens the library inside itself, or the path `XPLPC_LIBRARY_PATH` names, and never walks into this repository's build directory.
- The serializer encodes any object that carries `to_json` wherever it sits in a value, through the `default` hook of `json.dumps`.
