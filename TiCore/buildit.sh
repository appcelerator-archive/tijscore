xcodebuild -sdk iphonesimulator -configuration Release -target iOSTiCore clean
xcodebuild -sdk iphoneos -configuration Release -target iOSTiCore clean

xcodebuild -sdk iphonesimulator -configuration Release -target iOSTiCore
xcodebuild -sdk iphoneos -configuration Release -target iOSTiCore

lipo build/Release-iphonesimulator/libiOSTiCore.a build/Release-iphoneos/libiOSTiCore.a -create -output build/libTiCore.a
