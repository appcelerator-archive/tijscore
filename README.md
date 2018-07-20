# TiCoreJS [![Build Status](https://travis-ci.org/appcelerator/tijscore.svg?branch=v20)](https://travis-ci.org/appcelerator/tijscore)

> ⚠️This project has been deprecated in favor of the builtin JavaScriptCore framework. Titanium SDK 8+ will not
include this library anymore.

This is a Titanium Mobile fork of WebKit KJS. All changes are made available under the Apache Public License (version 2).  

To build first run the WTF script followed by the TiCore script.

The built library will be in the Build Folder (generated) along with public and private headers from the WTF and JavaScriptCore projects (needed for Debugger/Profiler/Titanium)

We are only generating i386, x86_64, armv7 and arm64 slices in the static library (no armv7s)
