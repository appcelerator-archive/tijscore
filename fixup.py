#!/usr/bin/env python
#
#
# This is a script that will attempt to fix up filenames
# and symbols inside files to match the Titanium namespace,
# naming conventions, etc so as not to conflict with an
# existing install of KJS
#
import os, shutil, sys, re

if len(sys.argv) > 1:
	root_dir = sys.argv[1]
else:
	root_dir = "TiCore"


tokens = [
	['JSString','TiString'],
	['JavaScript','Ti'],
	['JSRetain','TiRetain'],
	['JSRelease','TiRelease'],
	['JSObject','TiObject'],
	['JSLock','TiLock'],
	['JSUnlock','TiUnlock'],
	['JSCell','TiCell'],
	['JSClass','TiClass'],
	['JSStatic','TiStatic'],
	['JSContext','TiContext'],
	['JSGlobal','TiGlobal'],
	['JSValue','TiValue'],
	['JSArray','TiArray'],
	['JSByte','TiArray'],
	['JSFunction','TiFunction'],
	['JSProperty','TiProperty'],
	['ExecState','TiExcState'],
	['JSGlobalData','TiGlobalData'],
	['kJS','kTI'],
	['JSEvaluate','TiEval'],
	['JSCheck','TiCheck'],
	['JSGarbage','TiGarbage'],
	['JSType','TiType'],
	['JSAPI','TiAPI'],
	['JSCallback','TiCallback'],
	['JSProfile','TiProfile'],
	['JSBase','TiBase'],
	['JSChar','TiChar'],
	['WTFMain','WTIMain'],
]

#['jsAPIValueWrapper','TiAPIValueWrapper'],

COPYRIGHT_NOW = "2012"
COPYRIGHT = """/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-%s by Appcelerator, Inc.
 */

""" % COPYRIGHT_NOW

def fix_copyright(ext,content):
	if ext in ('.h','.cpp','.c','.mm'):
		if content.find('Appcelerator Titanium License')==-1:
			content = COPYRIGHT + content
		else:
			# update the copyright information if necessary
			content = re.sub('Copyright \(c\) 2009\S*', 'Copyright (c) 2009-%s' % COPYRIGHT_NOW, content)
			
	return content


def fix_content(fn):
	content = open(fn,'r').read()
	if os.path.basename(fn)=='project.pbxproj':
		content = content.replace('build/Release/DerivedSources/JavaScriptCore,','"$(BUILD_DIR)/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)/DerivedSources/JavaScriptCore",')
	for token in tokens:
		content = content.replace(token[0],token[1])
	content = content.replace('namespace JSC','namespace TI')
	content = content.replace('namespace WTF','namespace WTI')
	content = content.replace('using WTF::','using WTI::')
	content = content.replace('using JSC::','using TI::')
	content = content.replace('using WTFNoncopyable::','using WTINoncopyable::')
	content = content.replace('JSC::','TI::')
	content = content.replace('WTF::','WTI::')
	return content
	
def fix_filename(fn):
	dirname = os.path.dirname(fn)
	path = os.path.basename(fn)
	ext = os.path.splitext(path)[1]
	if ext in ('.c','.cpp','.mm','.h','.pbxproj','.exp','.xcconfig','.sh','.make','.y','.lut.h') or path == 'create_hash_table':
		found = False
		content = fix_content(fn)
		content = fix_copyright(ext,content)
		for token in tokens:
			if token[0] in path:
				newfn = os.path.join(dirname,path.replace(token[0],token[1]))
				found = True
				newf = open(newfn,'w')
				newf.write(content)
				newf.close()
				print "Renamed: %s" % newfn
				os.remove(fn)
				break
		if not found:
			newf = open(fn,'w')
			newf.write(content)
			newf.close()
			print "Fixed: %s" % fn
		return True
	return False



for root, dirs, files in os.walk(os.path.abspath(root_dir)):
	for file in files:
		from_ = os.path.join(root, file)
		#print from_			  
		fix_filename(from_)

xcode = os.path.join(root_dir,'JavaScriptCore.xcodeproj')
if os.path.exists(xcode):
	newxcode = os.path.join(root_dir,'TiCore.xcodeproj')
	os.rename(xcode,newxcode)

jsc = os.path.join(root_dir,'DerivedSources','JavaScriptCore')
if os.path.exists(jsc):
	newjsc = os.path.join(root_dir,'DerivedSources','TiCore')
	os.rename(jsc,newjsc)

sys.exit(0)
