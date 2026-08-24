from core.targets import swift_ios, swift_macos

# a single xcframework has to carry every apple slice, since a swift package declares one binary target
data = swift_ios.data + swift_macos.data
