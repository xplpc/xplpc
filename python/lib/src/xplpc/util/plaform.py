import platform


# -----------------------------------------------------------------------------
def is_windows():
    return any(platform.win32_ver())


# -----------------------------------------------------------------------------
def is_macos():
    return platform.system().lower().startswith("darwin")


# -----------------------------------------------------------------------------
def is_linux():
    return platform.system().lower().startswith("linux")


# -----------------------------------------------------------------------------
def get_lib_binary_dir():
    if is_windows():
        return "bin"

    return "lib"
