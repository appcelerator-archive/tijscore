#!/usr/bin/env python
#
#
# This is a script that will attempt to fix up filenames
# and symbols inside files to match the Titanium namespace,
# naming conventions, etc so as not to conflict with an
# existing install of KJS
#
import os, shutil, sys, re, datetime

if len(sys.argv) > 1:
	root_dir = sys.argv[1]
else:
	root_dir = "TiCore"


tokens = [
	['kJSProperty','kTiProperty'],
	['JSClass','TiClass'],
	['kJS', 'kTI'],
	['JSStringRef','TiStringRef'],
	['JSObjectRef','TiObjectRef'],
	['JSValue','TiValue'],
	['JSContext','TiContext'],
	['OpaqueJSString', 'OpaqueTiString'],
	['OpaqueJSClass', 'OpaqueTiClass'],
	['JSGlobalContext','TiGlobalContext'],
	['JSEvaluate','TiEval'],
	['JSCheck','TiCheck'],
	['JSGarbage','TiGarbage'],
	['JSBase','TiBase'],
	['JSExport','TiExport'],
	['JSManagedValue','TiManagedValue'],
	['JSVirtualMachine','TiVirtualMachine'],
	['JavaScript.h','Ti.h'],
	['JavaScriptCore.h','TiCore.h'],
	['JavaScriptCore_h','TiCore_h'],
	['JavaScript_h','Ti_h'],

	['JSPropertyAttributes', 'TiPropertyAttributes'],
	['JSClassAttributes', 'TiClassAttributes'],
	['JSObjectInitializeCallback', 'TiObjectInitializeCallback'],
	['JSObjectFinalizeCallback', 'TiObjectFinalizeCallback'],
	['JSObjectConvertToTypeCallback','TiObjectConvertToTypeCallback'],
	['JSStaticValue','TiStaticValue'],
	['JSStaticFunction','TiStaticFunction'],
	['JSPropertyNameArray','TiPropertyNameArray'],
	['JSPropertyNameAccumulator','TiPropertyNameAccumulator'],

	['JSObjectMake','TiObjectMake'],
	['JSObjectGet','TiObjectGet'],
	['JSObjectSet','TiObjectSet'],
	['JSObjectHas','TiObjectHas'],
	['JSObjectDelete','TiObjectDelete'],
	['JSObjectIs','TiObjectIs'],
	['JSObjectCall','TiObjectCall'],
	['JSObjectCopy','TiObjectCopy'],

	['JSStringCreateWithCharacters','TiStringCreateWithCharacters'],
	['JSStringCreateWithUTF8CString','TiStringCreateWithUTF8CString'],
	['JSStringRetain','TiStringRetain'],
	['JSStringRelease','TiStringRelease'],
	['JSStringGetLength','TiStringGetLength'],
	['JSStringGetCharactersPtr','TiStringGetCharactersPtr'],
	['JSStringGetMaximumUTF8CStringSize','TiStringGetMaximumUTF8CStringSize'],
	['JSStringGetUTF8CString','TiStringGetUTF8CString'],
	['JSStringIsEqual','TiStringIsEqual'],
	['JSStringIsEqualToUTF8CString','TiStringIsEqualToUTF8CString'],
	['JSStringCreateWithCFString','TiStringCreateWithCFString'],
	['JSStringCopyCFString','TiStringCopyCFString'],
	['WTFMain','WTIMain'],
]

extensions = (
	'.c',
	'.cc',
	'.cpp',
	'.mm',
	'.h',
	'.pbxproj',
	'.exp',
	'.xcconfig',
	'.sh',
	'.make',
	'.y',
	'.lut.h',
	'.py'
)

copyright_ext = (
	'.h',
	'.cpp',
	'.c',
	'.cc',
	'.mm'
)

#['jsAPIValueWrapper','TiAPIValueWrapper'],

COPYRIGHT_NOW = datetime.date.today().strftime('%Y')
COPYRIGHT = """/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-%s by Appcelerator, Inc.
 */

""" % COPYRIGHT_NOW

def fix_copyright(ext,content):
	if ext in copyright_ext:
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
	content = content.replace('<TiCore/','<JavaScriptCore/')
	return content
	
def fix_filename(fn):
	dirname = os.path.dirname(fn)
	path = os.path.basename(fn)
	ext = os.path.splitext(path)[1]
	if ext in extensions or path == 'create_hash_table':
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

sys.exit(0)
