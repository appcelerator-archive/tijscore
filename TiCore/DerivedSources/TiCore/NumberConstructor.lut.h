/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

// Automatically generated from ../../runtime/NumberConstructor.cpp using ../../create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace TI {

static const struct HashTableValue numberTableValues[6] = {
   { "NaN", DontEnum|DontDelete|ReadOnly, (intptr_t)numberConstructorNaNValue, (intptr_t)0 },
   { "NEGATIVE_INFINITY", DontEnum|DontDelete|ReadOnly, (intptr_t)numberConstructorNegInfinity, (intptr_t)0 },
   { "POSITIVE_INFINITY", DontEnum|DontDelete|ReadOnly, (intptr_t)numberConstructorPosInfinity, (intptr_t)0 },
   { "MAX_VALUE", DontEnum|DontDelete|ReadOnly, (intptr_t)numberConstructorMaxValue, (intptr_t)0 },
   { "MIN_VALUE", DontEnum|DontDelete|ReadOnly, (intptr_t)numberConstructorMinValue, (intptr_t)0 },
   { 0, 0, 0, 0 }
};

extern JSC_CONST_HASHTABLE HashTable numberTable =
    { 16, 15, numberTableValues, 0 };
} // namespace
