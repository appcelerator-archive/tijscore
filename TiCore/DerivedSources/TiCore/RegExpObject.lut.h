/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

// Automatically generated from ../../runtime/RegExpObject.cpp using ../../create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace TI {
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const struct HashTableValue regExpTableValues[6] = {
   { "global", DontDelete|ReadOnly|DontEnum, (intptr_t)static_cast<PropertySlot::GetValueFunc>(regExpObjectGlobal), (intptr_t)0 THUNK_GENERATOR(0) },
   { "ignoreCase", DontDelete|ReadOnly|DontEnum, (intptr_t)static_cast<PropertySlot::GetValueFunc>(regExpObjectIgnoreCase), (intptr_t)0 THUNK_GENERATOR(0) },
   { "multiline", DontDelete|ReadOnly|DontEnum, (intptr_t)static_cast<PropertySlot::GetValueFunc>(regExpObjectMultiline), (intptr_t)0 THUNK_GENERATOR(0) },
   { "source", DontDelete|ReadOnly|DontEnum, (intptr_t)static_cast<PropertySlot::GetValueFunc>(regExpObjectSource), (intptr_t)0 THUNK_GENERATOR(0) },
   { "lastIndex", DontDelete|DontEnum, (intptr_t)static_cast<PropertySlot::GetValueFunc>(regExpObjectLastIndex), (intptr_t)setRegExpObjectLastIndex THUNK_GENERATOR(0) },
   { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
extern JSC_CONST_HASHTABLE HashTable regExpTable =
    { 17, 15, regExpTableValues, 0 };
} // namespace
