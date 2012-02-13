#!/bin/sh

CONFIG="Release"
VERSION=

if (( $# > 0 )); then
    CONFIG=$1
fi

if (( $# > 1 )); then
    VERSION=$2
fi

xcodebuild -project TiCore/TiCore.xcodeproj -sdk iphonesimulator -configuration ${CONFIG} -target iOSTiCore clean
xcodebuild -project TiCore/TiCore.xcodeproj -sdk iphoneos -configuration ${CONFIG} -target iOSTiCore clean

xcodebuild -project TiCore/TiCore.xcodeproj -sdk iphonesimulator -configuration ${CONFIG} -target iOSTiCore
xcodebuild -project TiCore/TiCore.xcodeproj -sdk iphoneos -configuration ${CONFIG} -target iOSTiCore

lipo TiCore/build/${CONFIG}-iphonesimulator/libiOSTiCore.a TiCore/build/${CONFIG}-iphoneos/libiOSTiCore.a -create -output TiCore/build/libTiCore.a
if [[ ! -n "$VERSION" ]]; then
    # Package the ticore into a new version
    gzip -c TiCore/build/libTiCore.a > libTiCore-$VERSION.a.gz
fi