# WebAssembly (WASM)

All the WebAssembly client code use Typescript and is inside directory `wasm/sample/src/xplpc`.

## Requirements

* Node JS
* NPM
* Emscripten

## Setup EMSDK variables

Execute on terminal inside your EMSDK installation folder:

```
source emsdk_env.sh
echo $EMSDK
```

Sourcing it puts `emsdk` and `emcc` on the path and sets `EMSDK`, which every wasm task reads. Without it the task stops saying the variable is not set.

## How to build the library for WASM

Execute on terminal:

```
python3 xplpc.py wasm-build
```

or

```
cmake -S . -B build/wasm -DXPLPC_TARGET=wasm -DXPLPC_ADD_CUSTOM_DATA=ON -DCMAKE_TOOLCHAIN_FILE=${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build/wasm
```

## How to build the WASM sample

Execute on terminal:

```
python3 xplpc.py wasm-build-sample
```

or

```
cd wasm/sample
npm install
npm run build
```

## How to run the WASM sample

Execute on terminal:

```
python3 xplpc.py wasm-run-sample
```

or

```
cd wasm/sample
npm install
npm run dev
```

## How to serve the WASM sample

Execute on terminal:

```
python3 xplpc.py wasm-serve-sample
```

or

```
cd wasm/sample
npm install
npm run build
cd dist
python3 -m http.server
```

## How to test

Execute on terminal:

```
python3 xplpc.py wasm-test
```

or

```
cd wasm/sample
npm install
npm run test:unit
```

## How to format the WASM code

Execute on terminal:

```
python3 xplpc.py wasm-format
```

## Build parameters

You can build this target using some parameters to change configuration:

**Build type:**

```
--build debug
```

**Incremental, keeping what a previous build left:**

```
--incremental
```

**No dependencies:**

```
--no-deps
```

## Online demo

You can see the online demo on URL:

<https://xplpc.github.io/wasm-demo/>

## Taking it into your own application

There is no package registry for this bridge, so the TypeScript side is source you copy and the
module is what the build produced. Three things move:

| What | Where it comes from | Where it goes |
| --- | --- | --- |
| The library | `wasm/sample/src/xplpc` | Your source tree |
| The module | `build/wasm/wasm32/bin/xplpc.js` and `xplpc.wasm` | Wherever your bundler serves assets from |
| The declaration | `wasm/sample/types/xplpc.d.ts` | Your `typeRoots`, with the module path rewritten |

The sample reaches the module through a Vite alias that points at this repository, which is why its
imports read `@xplpc/build/...`. In your application the module is an ordinary asset, so the import
is whatever path your bundler serves it from and the declaration says the same path.

The library compiles as part of your bundle rather than shipping prebuilt, which is why the debug
level follows your own build rather than a runtime setting.

## Bringing the library up

The module is loaded first and handed to the library along with the serializer it carries:

```typescript
import Module from "../public/xplpc.js";

Module().then((module: WasmModule) => {
    XPLPC.shared().initialize(module, new Config(new JsonSerializer()));

    Mapping.initialize();
});
```

A mapping is registered by name, which is what makes it reachable from any other language:

```typescript
MappingList.shared().add(
    "platform.battery.level",
    new MappingItem(batteryLevel),
);
```

A call made before the module is up is reported and answered with nothing rather than waiting on a module that is not there.

## Making a call

A call is awaited, and the answer reaches you through the promise it returns:

```typescript
const request = new Request(
    "sample.login",
    new Param("username", "paulo"),
    new Param("password", "123456"),
    new Param("remember", true),
);

const response = await Client.call<string>(request);
console.log(response);
```

Control goes back to the event loop before the answer arrives, so nothing else on the page is held up. There is one entry point rather than two, since a promise is both shapes at once and takes a callback through `then`.

`callSync` answers with the value itself:

```typescript
const response = Client.callSync<string>(request);
console.log(response);
```

It reads an answer that is already there rather than waiting for one, so it works only when the mapping answers before it returns. A mapping that takes time is reported and answered with `undefined`, and the registration it would have resolved is dropped rather than left behind, so nothing is queued and nothing is leaked. It never blocks, which is why there is no timeout to pass and no deadlock to reach.

On this bridge that means the mappings the module owns and never the ones you wrote. A mapping here answers with a promise, which resolves in a microtask and so never inside the call, so `callSync` on a name this page owns always answers `undefined`. Reaching one of your own mappings is what `call` is for.

`undefined` is also what a mapping with nothing to answer produces, so the two are told apart by the console and not by the value.

## Writing a mapping

A mapping answers with a promise, so waiting is what it already does:

```typescript
function batteryLevel(): Promise<string> {
    return Promise.resolve("100%");
}
```

Anything that takes time is the same shape, since a promise is what the caller already waits on:

```typescript
function ip(): Promise<string> {
    return fetch("https://httpbin.io/ip").then((response) => response.text());
}
```

The module runs on the one thread JavaScript gave it, so a mapping that does slow synchronous work holds the page for as long as it takes. Returning a promise and letting the work finish on its own is what keeps that from happening.

## Sending a large buffer

`DataView` carries an address and a size across the bridge, so a large buffer crosses without being copied or encoded. Here the view owns the heap memory it copied into, so it is released with `DataView.free`, which ignores a view it did not allocate:

```ts
const view = DataView.createFromArrayBufferView(bytes);

try {
    const request = new Request("sample.image.grayscale.dataview", new Param("dataView", view));
    const response = await Client.call<string>(request);

    console.log(response ?? "");
} finally {
    DataView.free(view);
}
```

The bytes are never handed out, because the wasm heap grows into a new buffer and detaches every typed array taken from the old one, so a stale array reports length zero instead of failing. `withUint8Array` and `withUint8ClampedArray` build the array from the current buffer and pass it to a body where it cannot outlive the read:

```ts
DataView.withUint8Array(view, (bytes) => {
    console.log(bytes.length);
});
```

A view is released only by the side that allocated it, so a view handed to a mapping is never freed by that mapping, and a mapping that answers with a view has to keep its buffer alive after it returns.

## Logging

The library reports every failing path. An error names what failed and where, and is always reported. A debug line carries the reason behind it, including the message the parser produced, and is off until you ask for it.

Errors go to `console.error` and are always reported. The debug level is kept for a development build only, so `npm run dev` shows it and a production bundle does not.

Every line starts with `[XPLPC]`, which is what the other bridges get from a named logger and what the browser console has instead of one, so filtering on it in the devtools console shows or hides this library without touching your own.

## Sample project

You can see the sample project in directory `wasm/sample`.

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-wasm.png?raw=true">
