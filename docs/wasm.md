# Web Assembly (WASM)

On Web Assembly you have only the `LocalClient`, since it is not connected to other platform.

## Requirements

*  Node JS
*  NPM
*  Emscripten

## Setup EMSDK variables

Execute on terminal inside your EMSDK installation folder:

```
source emsdk_env.sh
echo $EMSDK
```

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

## Sample project

You can see the sample project in directory `wasm/sample`.

<img width="280" src="https://github.com/xplpc/xplpc/blob/main/extras/images/screenshot-wasm.png?raw=true">

Or you can see the demo online:

[https://xplpc.github.io/wasm-demo/](https://xplpc.github.io/wasm-demo/)
