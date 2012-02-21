/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

// Automatically generated from ../../runtime/ErrorPrototype.cpp using ../../create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace TI {
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const struct HashTableValue errorPrototypeTableValues[2] = {
   { "toString", DontEnum|Function, (intptr_t)static_cast<NativeFunction>(errorProtoFuncToString), (intptr_t)0 THUNK_GENERATOR(0) },
   { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
extern JSC_CONST_HASHTABLE HashTable errorPrototypeTable =
    { 2, 1, errorPrototypeTableValues, 0 };
} // namespace
