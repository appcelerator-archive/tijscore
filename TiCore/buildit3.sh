xcodebuild -project TiCore.xcodeproj -sdk iphonesimulator4.3 -target iOSTiCore -configuration Debug clean
xcodebuild -project TiCore.xcodeproj  -sdk iphoneos4.3 -target iOSTiCore -configuration Debug clean

xcodebuild -project TiCore.xcodeproj  -sdk iphonesimulator4.3 -target iOSTiCore -configuration Debug
xcodebuild -project TiCore.xcodeproj  -sdk iphoneos4.3 -target iOSTiCore -configuration Debug

lipo build/Debug-iphonesimulator/libiOSTiCore.a build/Debug-iphoneos/libiOSTiCore.a -create -output build/libTiCore.a
