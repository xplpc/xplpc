# frozen_string_literal: true

#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html.
# Run `pod lib lint xplpc.podspec` to validate before publishing.
#
Pod::Spec.new do |s|
  s.name             = 'xplpc'
  s.version          = '1.0.0'
  s.summary          = 'XPLPC Dart Plugin.'
  s.description      = 'XPLPC Dart Plugin Project.'
  s.homepage         = 'https://github.com/xplpc/xplpc'
  s.license          = { file: '../LICENSE' }
  s.author           = { 'Paulo Coutinho' => 'paulocoutinhox@gmail.com' }

  # Source is the path where the xcframework can be found.
  s.source = { path: '.' }
  s.dependency 'Flutter'
  s.frameworks = 'Foundation'

  # Flutter.framework does not contain a i386 slice.
  s.pod_target_xcconfig = { 'DEFINES_MODULE' => 'YES', 'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'i386' }
  s.swift_version = '5.0'

  # Versions.
  s.ios.deployment_target = '11.0'

  # Name of the xcframework.
  s.ios.vendored_frameworks = 'xplpc.xcframework'
end
