require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name         = "NitroFrameProcessor"
  s.version      = package["version"]
  s.summary      = package["description"]
  s.homepage     = package["homepage"]
  s.license      = package["license"]
  s.authors      = package["author"]

  s.platforms    = { :ios => min_ios_version_supported }
  s.source       = { :git => "https://github.com/PiotrSzczepanekVaveHealth/react-native-nitro-frame-processor.git", :tag => "#{s.version}" }

  s.source_files = [
    "ios/*.{swift,h,m,mm}",
    "cpp/**/*.{hpp,cpp}",
  ]
  s.public_header_files = "ios/CVIEBridge.h"
  s.vendored_frameworks = "ios/CVIESDK/bin/cvie64.framework"
  s.resources = ["ios/CVIESDK/par/*"]

  s.dependency 'React-jsi'
  s.dependency 'React-callinvoker'

  load 'nitrogen/generated/ios/NitroFrameProcessor+autolinking.rb'
  add_nitrogen_files(s)

  install_modules_dependencies(s)
end
