# frozen_string_literal: true

#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html.
# Run `pod lib lint xplpc.podspec` to validate before publishing.
#
Pod::Spec.new do |s|
  s.name             = 'xplpc_plugin'
  s.version          = '1.0.0'
  s.summary          = 'XPLPC Dart Plugin.'
  s.description      = 'XPLPC Dart Plugin Project.'
  s.homepage         = 'https://github.com/xplpc/xplpc'
  s.license          = { file: '../LICENSE' }
  s.author           = { 'Paulo Coutinho' => 'paulocoutinhox@gmail.com' }

  # Dependencies
  s.source = { path: '.' }
  s.dependency 'FlutterMacOS'
  s.frameworks = 'Foundation'

  # Flutter.framework does not contain a i386 slice.
  s.pod_target_xcconfig = { 'DEFINES_MODULE' => 'YES' }
  s.swift_version = '5.0'

  # Versions.
  s.platform = :osx, '10.11'

  # Name of the xcframework.
  s.vendored_frameworks = 'xplpc.xcframework'
end
