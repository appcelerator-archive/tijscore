xcodebuild -sdk iphonesimulator4.1 -activetarget -activeconfiguration clean
xcodebuild -sdk iphoneos4.1 -activetarget -activeconfiguration clean

xcodebuild -sdk iphonesimulator4.1 -activetarget -activeconfiguration
xcodebuild -sdk iphoneos4.1 -activetarget -activeconfiguration

python lipoit.py
