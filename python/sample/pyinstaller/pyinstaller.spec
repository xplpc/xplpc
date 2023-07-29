# -*- mode: python ; coding: utf-8 -*-

import os
import platform

# general
root_dir = os.path.abspath(os.path.join(os.getcwd(), ".."))
platform_name = platform.system().lower()
extras_dir = os.path.join(root_dir, "pyinstaller", "resources")

# platform data
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

# pyinstaller
block_cipher = None

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
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

pyz = PYZ(
    a.pure,
    a.zipped_data,
    cipher=block_cipher,
)

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
)

coll = COLLECT(
    exe,
    a.binaries,
    a.zipfiles,
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
