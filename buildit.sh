#!/bin/sh

CONFIG="Release"
VERSION=

if (( $# > 0 )); then
    CONFIG=$1
fi

if (( $# > 1 )); then
    VERSION=$2
fi

xcodebuild -project TiCore/TiCore.xcodeproj -sdk iphonesimulator -configuration ${CONFIG} -target TiCore clean
xcodebuild -project TiCore/TiCore.xcodeproj -sdk iphoneos -configuration ${CONFIG} -target TiCore clean

xcodebuild -project TiCore/TiCore.xcodeproj -sdk iphonesimulator -configuration ${CONFIG} -target TiCore
xcodebuild -project TiCore/TiCore.xcodeproj -sdk iphoneos -configuration ${CONFIG} -target TiCore

lipo TiCore/build/${CONFIG}-iphonesimulator/libTiCore.a TiCore/build/${CONFIG}-iphoneos/libTiCore.a -create -output TiCore/build/libTiCore.a
if [[ ! -z "$VERSION" ]]; then
    # Package the ticore into a new version
    gzip -c TiCore/build/libTiCore.a > libTiCore-$VERSION.a.gz
fi