xcodebuild -sdk iphonesimulator4.3 -activetarget -activeconfiguration clean
xcodebuild -sdk iphoneos4.3 -activetarget -activeconfiguration clean

xcodebuild -sdk iphonesimulator4.3 -activetarget -activeconfiguration
xcodebuild -sdk iphoneos4.3 -activetarget -activeconfiguration

python lipoit.py
