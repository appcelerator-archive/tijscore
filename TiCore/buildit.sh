#!/bin/sh

CONFIG="Release"

if (( $# > 0 )); then
    CONFIG=$1
fi

xcodebuild -sdk iphonesimulator -configuration ${CONFIG} -target iOSTiCore clean
xcodebuild -sdk iphoneos -configuration ${CONFIG} -target iOSTiCore clean

xcodebuild -sdk iphonesimulator -configuration ${CONFIG} -target iOSTiCore
xcodebuild -sdk iphoneos -configuration ${CONFIG} -target iOSTiCore

lipo build/${CONFIG}-iphonesimulator/libiOSTiCore.a build/${CONFIG}-iphoneos/libiOSTiCore.a -create -output build/libTiCore.a
