# Dependency Manager Tools

There are two supported tools for managing dependencies.

* CPM: You can find it at https://github.com/cpm-cmake/CPM.cmake.
* Conan: You can find it at https://github.com/conan-io/conan.

By default, the chosen dependency manager tool is `CPM`.

To use the other one, pass `--dependency-tool=<tool>` to a task that builds the native library, where the tool is `cpm` or `conan`. The consumer project always reaches the library through CPM, and the gradle, npm and flutter tasks have no native dependency of their own to resolve:

```
python3 xplpc.py cxx-test --dependency-tool=conan
```

When you switch the tool, all the scripts are already set up to use the correct tools with the necessary parameters.

In the `CMakeLists.txt` file, you can also observe how it works by looking at the `XPLPC_DEPENDENCY_TOOL_VALUES` variable.

Conan needs a profile before the first build, which this task creates:

```
python3 xplpc.py conan-setup
```

Conan takes the android ndk and the emscripten sdk from conan center, whose newest are r29 and 3.1.73, so an android or wasm build through conan compiles the dependencies with older tools than the build through CPM, which uses the ndk the android modules name and the emscripten sdk the wasm workflow installs.
