import os
import platform

root_dir = os.path.abspath(os.path.join(os.getcwd(), ".."))
platform_name = platform.system().lower()
extras_dir = os.path.join(root_dir, "pyinstaller", "resources")

program_name = "Runner"
icon_path = None
program_file = None

if platform_name == "darwin":
    icon_path = os.path.join(extras_dir, "macos", "icon.icns")
    program_file = "{0}.app".format(program_name)
elif platform_name == "linux":
    icon_path = os.path.join(extras_dir, "linux", "icon.png")
    program_file = "{0}".format(program_name)
elif platform_name == "windows":
    icon_path = os.path.join(extras_dir, "windows", "icon.ico")
    program_file = "{0}.exe".format(program_name)

a = Analysis(
    [
        os.path.join(
            root_dir,
            "src",
            "main.py",
        )
    ],
    pathex=[
        root_dir,
        os.path.join(root_dir, "src"),
    ],
    binaries=[],
    datas=[],
    hiddenimports=["xplpc", "kivymd.icon_definitions"],
    hookspath=["hooks"],
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
)

pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name=program_name,
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=False,
    icon=icon_path,
)

coll = COLLECT(
    exe,
    a.binaries,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name=program_name,
)

app = BUNDLE(
    coll,
    name=program_file,
    icon=icon_path,
    bundle_identifier="com.xplpc.runner",
)
