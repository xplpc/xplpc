from core.targets import swift_ios

# The flutter plugin ships only the iphone slices, and every field of them is declared once for the framework.
data = [item for item in swift_ios.data if item["group"] in ("ios", "ios-simulator")]
