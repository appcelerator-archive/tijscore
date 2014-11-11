#!/bin/sh

rm -rf Build
rm -rf WTF/build
mkdir Build
xcodebuild -project WTF/WTF.xcodeproj -sdk iphonesimulator -configuration "Release" -target WTF clean
xcodebuild -project WTF/WTF.xcodeproj -sdk iphoneos -configuration "Release" -target WTF clean
xcodebuild -project WTF/WTF.xcodeproj -sdk iphonesimulator -configuration "Release" -target WTF
xcodebuild -project WTF/WTF.xcodeproj -sdk iphoneos -configuration "Release" -target WTF
lipo WTF/build/Release-iphonesimulator/libWTF.a WTF/build/Release-iphoneos/libWTF.a -create -output Build/libWTF.a
xcrun -sdk iphoneos lipo -info Build/libWTF.a 

mkdir Build/PRIVATE_HEADERS
cp -R WTF/build/Release-iphoneos/usr/local/include/ Build/PRIVATE_HEADERS

rm -rf WTF/build
