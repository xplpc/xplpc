import platform

from core.targets import swift_macos

# The suite runs on the machine that builds it, so only the host slice is needed and its fields are declared once for the framework.
host_arch = "arm64" if platform.processor() == "arm" else "x86_64"

data = [item for item in swift_macos.data if item["arch"] == host_arch]
