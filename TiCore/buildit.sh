xcodebuild -sdk iphonesimulator3.2 -activetarget -activeconfiguration clean
xcodebuild -sdk iphoneos3.2 -activetarget -activeconfiguration clean

xcodebuild -sdk iphonesimulator3.2 -activetarget -activeconfiguration
xcodebuild -sdk iphoneos3.2 -activetarget -activeconfiguration

python lipoit.py
