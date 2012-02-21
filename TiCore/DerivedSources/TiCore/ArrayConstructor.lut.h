/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

// Automatically generated from ../../runtime/ArrayConstructor.cpp using ../../create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace TI {
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const struct HashTableValue arrayConstructorTableValues[2] = {
   { "isArray", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(arrayConstructorIsArray), (intptr_t)1 THUNK_GENERATOR(0) },
   { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
extern JSC_CONST_HASHTABLE HashTable arrayConstructorTable =
    { 2, 1, arrayConstructorTableValues, 0 };
} // namespace
