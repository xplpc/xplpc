# General

The scripts that execute the tasks are written in Python.

## Installation

### Using Virtual Environment (Recommended)

It's recommended to use a virtual environment to avoid conflicts with system packages:

    python3 -m venv .venv
    source .venv/bin/activate  # On Windows: .venv\Scripts\activate
    python3 -m pip install -r requirements.txt

### System Installation

You can also install Python requirements directly to your system:

    python3 -m pip install -r requirements.txt

## Testing Installation

You can test if everything is fine with the help command:

    python3 xplpc.py -h

## Formatter

To install the code formatters, you need install:

*   ClangFormat (C++)
*   Black (Python)
*   SwiftFormat (Swift)
*   Ktlint (Kotlin)
