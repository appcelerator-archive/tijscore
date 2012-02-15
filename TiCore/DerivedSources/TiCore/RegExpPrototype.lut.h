/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

// Automatically generated from ../../runtime/RegExpPrototype.cpp using ../../create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace TI {
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const struct HashTableValue regExpPrototypeTableValues[5] = {
   { "compile", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(regExpProtoFuncCompile), (intptr_t)2 THUNK_GENERATOR(0) },
   { "exec", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(regExpProtoFuncExec), (intptr_t)1 THUNK_GENERATOR(0) },
   { "test", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(regExpProtoFuncTest), (intptr_t)1 THUNK_GENERATOR(0) },
   { "toString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(regExpProtoFuncToString), (intptr_t)0 THUNK_GENERATOR(0) },
   { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
extern JSC_CONST_HASHTABLE HashTable regExpPrototypeTable =
    { 9, 7, regExpPrototypeTableValues, 0 };
} // namespace
