xcodebuild -sdk iphonesimulator3.1 -activetarget -activeconfiguration clean
xcodebuild -sdk iphoneos.1 -activetarget -activeconfiguration clean

xcodebuild -sdk iphonesimulator3.1 -activetarget -activeconfiguration
xcodebuild -sdk iphoneos.1 -activetarget -activeconfiguration

python lipoit.py
