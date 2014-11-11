#!/bin/sh

rm -rf JavaScriptCore/build
xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -sdk iphonesimulator -configuration "Release" -target JavaScriptCore clean
xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -sdk iphoneos -configuration "Release" -target JavaScriptCore clean
xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -sdk iphonesimulator -configuration "Release" -target JavaScriptCore
xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -sdk iphoneos -configuration "Release" -target JavaScriptCore
lipo JavaScriptCore/build/Release-iphonesimulator/libJavaScriptCore.a JavaScriptCore/build/Release-iphoneos/libJavaScriptCore.a -create -output Build/libTiCore.a
xcrun -sdk iphoneos lipo -info Build/libTiCore.a 

mkdir Build/PUBLIC_HEADERS
cp -R JavaScriptCore/build/Release-iphoneos/usr/local/include/ Build/PUBLIC_HEADERS
cp -R JavaScriptCore/build/Release-iphoneos/PRIVATE_HEADERS/ Build/PRIVATE_HEADERS

rm -rf JavaScriptCore/build
