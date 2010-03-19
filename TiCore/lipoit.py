#!/usr/bin/env python
#
#
# make a combined library
#

import os,sys

os.system("lipo build/Release-iphonesimulator/libStatic.a build/Release-iphoneos/libStatic.a -create -output build/libTiCore.a")
