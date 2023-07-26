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

## How to run the Python sample with PyInstaller

Before run the Python sample with PyInstaller, you need install Poetry packages with command:

    python3 -m pip install poetry

Build the Python package for Poetry find it locally on `build/python` folder:

    python3 xplpc.py python-build

Now execute PyInstaller:

    python3 xplpc.py python-pyinstaller

You can execute the binary inside folder `build/pyinstaller`.

## How to run the Python tests

Before run the Python tests, you need install the Python package.

Execute on terminal:

    python3 xplpc.py python-test

## How to format the Python code

Execute on terminal:

    python3 xplpc.py python-format
