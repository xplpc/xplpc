# frozen_string_literal: true

Pod::Spec.new do |s|
  s.name             = 'xplpc'
  s.version          = '1.0.0'
  s.summary          = 'XPLPC Dart Plugin.'
  s.description      = 'XPLPC Dart Plugin Project.'
  s.homepage         = 'https://github.com/xplpc/xplpc'
  s.license          = { file: '../LICENSE' }
  s.author           = { 'Paulo Coutinho' => 'paulocoutinhox@gmail.com' }

  s.source = { path: '.' }
  s.dependency 'Flutter'
  s.frameworks = 'Foundation'

  s.pod_target_xcconfig = { 'DEFINES_MODULE' => 'YES' }
  s.swift_version = '5.0'

  s.ios.deployment_target = '12.0'

  s.ios.vendored_frameworks = 'xplpc.xcframework'
end
