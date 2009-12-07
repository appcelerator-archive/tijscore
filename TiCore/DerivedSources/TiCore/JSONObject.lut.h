/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

// Automatically generated from ../../runtime/JSONObject.cpp using ../../create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace TI {

static const struct HashTableValue jsonTableValues[3] = {
   { "parse", DontEnum|Function, (intptr_t)JSONProtoFuncParse, (intptr_t)1 },
   { "stringify", DontEnum|Function, (intptr_t)JSONProtoFuncStringify, (intptr_t)1 },
   { 0, 0, 0, 0 }
};

extern JSC_CONST_HASHTABLE HashTable jsonTable =
    { 4, 3, jsonTableValues, 0 };
} // namespace
