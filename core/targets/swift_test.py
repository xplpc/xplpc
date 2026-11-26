from core import host
from core.targets import swift_macos

# The suite runs on the machine that builds it, so only the host slice is needed and its fields are declared once for the framework.
data = [item for item in swift_macos.data if item["arch"] == host.get_arch_path()]
