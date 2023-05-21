# Dependency Manager Tools

There are two supported tools for managing dependencies.

*   CPM: You can find it at https://github.com/cpm-cmake/CPM.cmake.
*   Conan: You can find it at https://github.com/conan-io/conan.

By default, the chosen dependency manager tool is `CPM`.

To change the dependency manager, you need to modify the `dependency_tool` variable in the `core/config.py` file.

When you switch the tool, all the scripts are already set up to use the correct tools with the necessary parameters.

In the `CMakeLists.txt` file, you can also observe how it works by looking at the `XPLPC_DEPENDENCY_TOOL_VALUES` variable.
