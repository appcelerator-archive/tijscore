#!/bin/sh
rm -rf build
mkdir build
mkdir build/TiCore

for arch in i386 x86_64; do
	echo "Cleaning WTF"
	xcodebuild -project WTF/WTF.xcodeproj -sdk iphonesimulator -configuration "Release" -target WTF clean > build/$arch.txt
	rm -rf WTF/build
	echo "Building WTF for $arch"
	xcodebuild -project WTF/WTF.xcodeproj -sdk iphonesimulator -configuration "Release" -target WTF -arch $arch >> build/$arch.txt
	echo "Copying libWTF.a"
	cp WTF/build/Release-iphonesimulator/libWTF.a build/libWTF.a
	echo "Cleaning JavaScriptCore"
	xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -sdk iphonesimulator -configuration "Release" -target JavaScriptCore clean >> build/$arch.txt
	rm -rf JavaScriptCore/build	
	echo "Building JavaScriptCore for $arch"
	xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -sdk iphonesimulator -configuration "Release" -target JavaScriptCore -arch $arch >> build/$arch.txt
	mkdir build/TiCore/$arch
	echo "Copying libJavaScriptCore.a"
	cp JavaScriptCore/build/Release-iphonesimulator/libJavaScriptCore.a build/TiCore/$arch/
	rm build/libWTF.a
done


for arch in armv7 arm64; do
	echo "Cleaning WTF"
	xcodebuild -project WTF/WTF.xcodeproj -sdk iphoneos -configuration "Release" -target WTF clean > build/$arch.txt
	rm -rf WTF/build
	echo "Building WTF for $arch"
	xcodebuild -project WTF/WTF.xcodeproj -sdk iphoneos -configuration "Release" -target WTF -arch $arch >> build/$arch.txt
	echo "Copying libWTF.a"
	cp WTF/build/Release-iphoneos/libWTF.a build/libWTF.a
	echo "Cleaning JavaScriptCore"
	xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -sdk iphoneos -configuration "Release" -target JavaScriptCore clean >> build/$arch.txt
	rm -rf JavaScriptCore/build	
	echo "Building JavaScriptCore for $arch"
	xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -sdk iphoneos -configuration "Release" -target JavaScriptCore -arch $arch >> build/$arch.txt
	mkdir build/TiCore/$arch
	echo "Copying libJavaScriptCore.a"
	cp JavaScriptCore/build/Release-iphoneos/libJavaScriptCore.a build/TiCore/$arch/
	rm build/libWTF.a
done

echo "Creating Universal Binary"

lipo build/TiCore/i386/libJavaScriptCore.a build/TiCore/x86_64/libJavaScriptCore.a build/TiCore/armv7/libJavaScriptCore.a build/TiCore/arm64/libJavaScriptCore.a -create -output build/libTiCore.a

for arch in armv7 arm64 i386 x86_64; do
	xcrun -sdk iphoneos lipo build/libTiCore.a -verify_arch $arch
	if (( $? != 0 )); then
		echo "ERROR: YOU DID NOT BUILD IN SYMBOLS FOR $arch"
		exit 1
	fi
done

echo "Checking Universal Binary"
xcrun -sdk iphoneos lipo -info build/libTiCore.a

echo "Copying Headers"

mkdir build/PRIVATE_HEADERS
cp -R WTF/build/Release-iphoneos/usr/local/include/ build/PRIVATE_HEADERS
mkdir build/PUBLIC_HEADERS
mkdir build/PUBLIC_HEADERS/JavaScriptCore
cp -R JavaScriptCore/build/Release-iphoneos/usr/local/include/ build/PUBLIC_HEADERS/JavaScriptCore
cp -R JavaScriptCore/build/Release-iphoneos/PRIVATE_HEADERS/ build/PRIVATE_HEADERS

echo "Done"
