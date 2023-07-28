# Python

## Requirements

*   Python version 3.7 or later (required due to the necessary support for asyncio)

## How to build the shared library

Execute on terminal:

    python3 xplpc.py c-build-shared

or

    cmake -S . -B build/c-shared -DXPLPC_TARGET=c-shared -DXPLPC_ADD_CUSTOM_DATA=ON
    cmake --build build/c-shared

## How to install the Python package

Before install and build the Python package, you need build the C shared library.

To utilize the default behavior, which installs the development package, execute the following command in the terminal:

    python3 xplpc.py python-install

However, if you wish to create and install a wheel package (.whl), please follow these steps:

1.  Set the `use_dev` variable inside `core/python.py` to `False`.
2.  Run the following command in your terminal to build: `python3 xplpc.py python-build`
3.  The generated `.whl` file can be found at `build/python/dist/xplpc-1.0.0-py3-none-any.whl`
4.  To install it, execute this command in your terminal: `python3 xplpc.py python-install`

## How to run the Python sample

Before run the Python sample, you need install the Python package.

Install the sample dependencies:

    python3 -m pip install -r python/sample/src/requirements.txt

Run the sample executing on terminal:

    python3 xplpc.py python-sample

## How to run the Python sample with PyInstaller

Because of some problems on PyInstaller to identify the `xplpc` Python libraries, you need be inside another environment to work.

Build the Python package for Poetry find it locally on `build/python` folder:

    python3 xplpc.py python-build

Before run the Python sample with PyInstaller, you need install Poetry packages with command:

    python3 -m pip install poetry

Execute PyInstaller:

    python3 xplpc.py python-pyinstaller

You can execute the binary inside folder `build/pyinstaller`.

## How to run the Python tests

Before run the Python tests, you need install the Python package.

Execute on terminal:

    python3 xplpc.py python-test

## How to format the Python code

Execute on terminal:

    python3 xplpc.py python-format
