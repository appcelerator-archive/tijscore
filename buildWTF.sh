#!/bin/sh

rm -rf build
rm -rf WTF/build
mkdir build
xcodebuild -project WTF/WTF.xcodeproj -sdk iphonesimulator -configuration "Release" -target WTF clean
xcodebuild -project WTF/WTF.xcodeproj -sdk iphoneos -configuration "Release" -target WTF clean
xcodebuild -project WTF/WTF.xcodeproj -sdk iphonesimulator -configuration "Release" -target WTF OTHER_CFLAGS="-fembed-bitcode" CLANG_ENABLE_MODULE_DEBUGGING=NO GCC_PRECOMPILE_PREFIX_HEADER=NO DEBUG_INFORMATION_FORMAT="DWARF with dSYM"
xcodebuild -project WTF/WTF.xcodeproj -sdk iphoneos -configuration "Release" -target WTF OTHER_CFLAGS="-fembed-bitcode" CLANG_ENABLE_MODULE_DEBUGGING=NO GCC_PRECOMPILE_PREFIX_HEADER=NO DEBUG_INFORMATION_FORMAT="DWARF with dSYM"
lipo WTF/build/Release-iphonesimulator/libWTF.a WTF/build/Release-iphoneos/libWTF.a -create -output build/libWTF.a

for arch in armv7 arm64 i386 x86_64; do
	xcrun -sdk iphoneos lipo build/libWTF.a -verify_arch $arch
	if (( $? != 0 )); then
		echo "ERROR: YOU DID NOT BUILD IN SYMBOLS FOR $arch"
		exit 1
	fi
done

xcrun -sdk iphoneos lipo -info build/libWTF.a

mkdir Build/PRIVATE_HEADERS
cp -R WTF/build/Release-iphoneos/usr/local/include/ build/PRIVATE_HEADERS

rm -rf WTF/build
