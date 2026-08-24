# General

The scripts that execute the tasks are written in Python.

## Installation

### Using Virtual Environment (Recommended)

It's recommended to use a virtual environment to avoid conflicts with system packages:

```
python3 -m venv .venv
source .venv/bin/activate  # On Windows: .venv\Scripts\activate
python3 -m pip install -r requirements.txt
```

### System Installation

You can also install Python requirements directly to your system:

```
python3 -m pip install -r requirements.txt
```

## Testing Installation

You can test if everything is fine with the help command:

```
python3 xplpc.py -h
```

## Formatter

Every language has a format task, and each formatter comes from a different place:

| Task | Formatter | Where it comes from |
| --- | --- | --- |
| `cxx-format`, `c-format` | ClangFormat | Xcode or LLVM |
| `python-format` | Black | `requirements.txt` |
| `swift-format` | SwiftFormat | `Brewfile` |
| `kotlin-format` | Ktlint | `Brewfile` |
| `wasm-format` | Prettier | the sample's npm dependencies |
| `flutter-format` | dart format | the Flutter SDK |
| `docs-format` | Remark | `npm install -g remark-cli@12.0.1` |

Three languages also have a linter, which checks what a formatter cannot:

| Task | Linter | Where it comes from |
| --- | --- | --- |
| `swift-lint` | SwiftLint | `Brewfile` |
| `kotlin-lint` | Detekt | `Brewfile` |
| `python-lint` | Ruff and Mypy | `requirements.txt` |

Android carries a second analyzer of its own, since the platform ships one for the aar and the apk
that Detekt does not replace, and the Android workflow runs it. `python-lint` also checks the types
the package declares, because an annotation nothing verifies is worth less than none.

Every linter runs in the workflow of its own language. Three of the formatters do too, since Black, Prettier and Remark are the ones whose version is pinned, and a job runs them and fails on any diff. The other four come from the system or from Brew, where a version cannot be pinned, so a check on them would go red on an image bump rather than on a mistake.

The four that come from the `Brewfile` are installed with:

```
python3 xplpc.py brew
```
