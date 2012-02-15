/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

// Automatically generated from ../../runtime/JSONObject.cpp using ../../create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace TI {
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const struct HashTableValue jsonTableValues[3] = {
   { "parse", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(JSONProtoFuncParse), (intptr_t)2 THUNK_GENERATOR(0) },
   { "stringify", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(JSONProtoFuncStringify), (intptr_t)3 THUNK_GENERATOR(0) },
   { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
extern JSC_CONST_HASHTABLE HashTable jsonTable =
    { 4, 3, jsonTableValues, 0 };
} // namespace
