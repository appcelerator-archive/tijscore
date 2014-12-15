#!/bin/sh

rm -rf JavaScriptCore/build
xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -sdk iphonesimulator -configuration "Release" -target JavaScriptCore clean
xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -sdk iphoneos -configuration "Release" -target JavaScriptCore clean
xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -sdk iphonesimulator -configuration "Release" -target JavaScriptCore
xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -sdk iphoneos -configuration "Release" -target JavaScriptCore
lipo JavaScriptCore/build/Release-iphonesimulator/libJavaScriptCore.a JavaScriptCore/build/Release-iphoneos/libJavaScriptCore.a -create -output build/libTiCore.a

for arch in armv7 arm64 i386 x86_64; do
	xcrun -sdk iphoneos lipo build/libTiCore.a -verify_arch $arch
	if (( $? != 0 )); then
		echo "ERROR: YOU DID NOT BUILD IN SYMBOLS FOR $arch"
		exit 1
	fi
done

xcrun -sdk iphoneos lipo -info build/libTiCore.a

mkdir build/PUBLIC_HEADERS
mkdir build/PUBLIC_HEADERS/JavaScriptCore
cp -R JavaScriptCore/build/Release-iphoneos/usr/local/include/ build/PUBLIC_HEADERS/JavaScriptCore
cp -R JavaScriptCore/build/Release-iphoneos/PRIVATE_HEADERS/ build/PRIVATE_HEADERS

rm -rf JavaScriptCore/build
