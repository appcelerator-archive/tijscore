#!/bin/sh

mkdir -p "${BUILT_PRODUCTS_DIR}/DerivedSources/TiCore/docs"
cd "${BUILT_PRODUCTS_DIR}/DerivedSources/TiCore"

/bin/ln -sfh "${SRCROOT}/.." TiCore
export TiCore="TiCore"

make --no-builtin-rules -f "TiCore/DerivedSources.make" -j `/usr/sbin/sysctl -n hw.ncpu`
