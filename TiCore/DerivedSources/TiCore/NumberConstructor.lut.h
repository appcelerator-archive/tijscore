/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

// Automatically generated from ../../runtime/NumberConstructor.cpp using ../../create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace TI {
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const struct HashTableValue numberConstructorTableValues[6] = {
   { "NaN", DontEnum|DontDelete|ReadOnly, (intptr_t)static_cast<PropertySlot::GetValueFunc>(numberConstructorNaNValue), (intptr_t)0 THUNK_GENERATOR(0) },
   { "NEGATIVE_INFINITY", DontEnum|DontDelete|ReadOnly, (intptr_t)static_cast<PropertySlot::GetValueFunc>(numberConstructorNegInfinity), (intptr_t)0 THUNK_GENERATOR(0) },
   { "POSITIVE_INFINITY", DontEnum|DontDelete|ReadOnly, (intptr_t)static_cast<PropertySlot::GetValueFunc>(numberConstructorPosInfinity), (intptr_t)0 THUNK_GENERATOR(0) },
   { "MAX_VALUE", DontEnum|DontDelete|ReadOnly, (intptr_t)static_cast<PropertySlot::GetValueFunc>(numberConstructorMaxValue), (intptr_t)0 THUNK_GENERATOR(0) },
   { "MIN_VALUE", DontEnum|DontDelete|ReadOnly, (intptr_t)static_cast<PropertySlot::GetValueFunc>(numberConstructorMinValue), (intptr_t)0 THUNK_GENERATOR(0) },
   { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
extern JSC_CONST_HASHTABLE HashTable numberConstructorTable =
    { 16, 15, numberConstructorTableValues, 0 };
} // namespace
