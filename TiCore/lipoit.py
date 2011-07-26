#!/usr/bin/env python
#
#
# make a combined library
#

import os,sys

os.system("lipo build/Release-iphonesimulator/libiOSTiCore.a build/Release-iphoneos/libiOSTiCore.a -create -output build/libTiCore.a")
