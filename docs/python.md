# Python

## How to build the library for C

Execute on terminal:

    python3 xplpc.py c-build-shared

or

    cmake -S . -B build/c-shared -DXPLPC_TARGET=c-shared -DXPLPC_ADD_CUSTOM_DATA=ON
    cmake --build build/c-shared

## How to build the Python package

Before build the Python package, you need build the C shared library.

Execute on terminal:

    python3 xplpc.py python-build

## How to install the Python package

Before install the Python package, you need build the Python package.

Execute on terminal:

    python3 xplpc.py python-install

## How to run the Python sample

Before run the Python sample, you need install the Python package.

Execute on terminal:

    python3 xplpc.py python-sample

## How to run the Python tests

Before run the Python tests, you need install the Python package.

Execute on terminal:

    python3 xplpc.py python-test

## How to format the Python code

Execute on terminal:

    python3 xplpc.py python-format
