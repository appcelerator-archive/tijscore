/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
 *  Copyright (C) 2003 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "ArrayPrototype.h"

#include "CachedCall.h"
#include "CodeBlock.h"
#include "Interpreter.h"
#include "JIT.h"
#include "TiStringBuilder.h"
#include "Lookup.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "StringRecursionChecker.h"
#include <algorithm>
#include <wtf/Assertions.h>
#include <wtf/HashSet.h>

namespace TI {

ASSERT_CLASS_FITS_IN_CELL(ArrayPrototype);

static EncodedTiValue JSC_HOST_CALL arrayProtoFuncToString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncToLocaleString(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncConcat(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncJoin(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncPop(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncPush(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncReverse(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncShift(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncSlice(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncSort(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncSplice(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncUnShift(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncEvery(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncForEach(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncSome(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncIndexOf(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncFilter(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncMap(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncReduce(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncReduceRight(TiExcState*);
static EncodedTiValue JSC_HOST_CALL arrayProtoFuncLastIndexOf(TiExcState*);

}

#include "ArrayPrototype.lut.h"

namespace TI {

static inline bool isNumericCompareFunction(TiExcState* exec, CallType callType, const CallData& callData)
{
    if (callType != CallTypeJS)
        return false;

    FunctionExecutable* executable = callData.js.functionExecutable;

    TiObject* error = executable->compileForCall(exec, callData.js.scopeChain);
    if (error)
        return false;

    return executable->generatedBytecodeForCall().isNumericCompareFunction();
}

// ------------------------------ ArrayPrototype ----------------------------

const ClassInfo ArrayPrototype::s_info = {"Array", &TiArray::s_info, 0, TiExcState::arrayPrototypeTable};

/* Source for ArrayPrototype.lut.h
@begin arrayPrototypeTable 16
  toString       arrayProtoFuncToString       DontEnum|Function 0
  toLocaleString arrayProtoFuncToLocaleString DontEnum|Function 0
  concat         arrayProtoFuncConcat         DontEnum|Function 1
  join           arrayProtoFuncJoin           DontEnum|Function 1
  pop            arrayProtoFuncPop            DontEnum|Function 0
  push           arrayProtoFuncPush           DontEnum|Function 1
  reverse        arrayProtoFuncReverse        DontEnum|Function 0
  shift          arrayProtoFuncShift          DontEnum|Function 0
  slice          arrayProtoFuncSlice          DontEnum|Function 2
  sort           arrayProtoFuncSort           DontEnum|Function 1
  splice         arrayProtoFuncSplice         DontEnum|Function 2
  unshift        arrayProtoFuncUnShift        DontEnum|Function 1
  every          arrayProtoFuncEvery          DontEnum|Function 1
  forEach        arrayProtoFuncForEach        DontEnum|Function 1
  some           arrayProtoFuncSome           DontEnum|Function 1
  indexOf        arrayProtoFuncIndexOf        DontEnum|Function 1
  lastIndexOf    arrayProtoFuncLastIndexOf    DontEnum|Function 1
  filter         arrayProtoFuncFilter         DontEnum|Function 1
  reduce         arrayProtoFuncReduce         DontEnum|Function 1
  reduceRight    arrayProtoFuncReduceRight    DontEnum|Function 1
  map            arrayProtoFuncMap            DontEnum|Function 1
@end
*/

// ECMA 15.4.4
ArrayPrototype::ArrayPrototype(TiGlobalObject* globalObject, Structure* structure)
    : TiArray(globalObject->globalData(), structure)
{
    ASSERT(inherits(&s_info));
    putAnonymousValue(globalObject->globalData(), 0, globalObject);
}

bool ArrayPrototype::getOwnPropertySlot(TiExcState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticFunctionSlot<TiArray>(exec, TiExcState::arrayPrototypeTable(exec), this, propertyName, slot);
}

bool ArrayPrototype::getOwnPropertyDescriptor(TiExcState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<TiArray>(exec, TiExcState::arrayPrototypeTable(exec), this, propertyName, descriptor);
}

// ------------------------------ Array Functions ----------------------------

// Helper function
static TiValue getProperty(TiExcState* exec, TiObject* obj, unsigned index)
{
    PropertySlot slot(obj);
    if (!obj->getPropertySlot(exec, index, slot))
        return TiValue();
    return slot.getValue(exec, index);
}

static void putProperty(TiExcState* exec, TiObject* obj, const Identifier& propertyName, TiValue value)
{
    PutPropertySlot slot;
    obj->put(exec, propertyName, value, slot);
}

static unsigned argumentClampedIndexFromStartOrEnd(TiExcState* exec, int argument, unsigned length, unsigned undefinedValue = 0)
{
    TiValue value = exec->argument(argument);
    if (value.isUndefined())
        return undefinedValue;

    double indexDouble = value.toInteger(exec);
    if (indexDouble < 0) {
        indexDouble += length;
        return indexDouble < 0 ? 0 : static_cast<unsigned>(indexDouble);
    }
    return indexDouble > length ? length : static_cast<unsigned>(indexDouble);
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncToString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();

    bool isRealArray = isTiArray(&exec->globalData(), thisValue);
    if (!isRealArray && !thisValue.inherits(&TiArray::s_info))
        return throwVMTypeError(exec);
    TiArray* thisObj = asArray(thisValue);
    
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    StringRecursionChecker checker(exec, thisObj);
    if (EncodedTiValue earlyReturnValue = checker.earlyReturnValue())
        return earlyReturnValue;

    unsigned totalSize = length ? length - 1 : 0;
#if OS(SYMBIAN)
    // Symbian has very limited stack size available.
    // This function could be called recursively and allocating 1K on stack here cause
    // stack overflow on Symbian devices.
    Vector<RefPtr<StringImpl> > strBuffer(length);
#else
    Vector<RefPtr<StringImpl>, 256> strBuffer(length);
#endif    
    for (unsigned k = 0; k < length; k++) {
        TiValue element;
        if (isRealArray && thisObj->canGetIndex(k))
            element = thisObj->getIndex(k);
        else
            element = thisObj->get(exec, k);
        
        if (element.isUndefinedOrNull())
            continue;
        
        UString str = element.toString(exec);
        strBuffer[k] = str.impl();
        totalSize += str.length();
        
        if (!strBuffer.data()) {
            throwOutOfMemoryError(exec);
        }
        
        if (exec->hadException())
            break;
    }
    if (!totalSize)
        return TiValue::encode(jsEmptyString(exec));
    Vector<UChar> buffer;
    buffer.reserveCapacity(totalSize);
    if (!buffer.data())
        return TiValue::encode(throwOutOfMemoryError(exec));
        
    for (unsigned i = 0; i < length; i++) {
        if (i)
            buffer.append(',');
        if (RefPtr<StringImpl> rep = strBuffer[i])
            buffer.append(rep->characters(), rep->length());
    }
    ASSERT(buffer.size() == totalSize);
    return TiValue::encode(jsString(exec, UString::adopt(buffer)));
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncToLocaleString(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();

    if (!thisValue.inherits(&TiArray::s_info))
        return throwVMTypeError(exec);
    TiObject* thisObj = asArray(thisValue);

    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    StringRecursionChecker checker(exec, thisObj);
    if (EncodedTiValue earlyReturnValue = checker.earlyReturnValue())
        return earlyReturnValue;

    TiStringBuilder strBuffer;
    for (unsigned k = 0; k < length; k++) {
        if (k >= 1)
            strBuffer.append(',');

        TiValue element = thisObj->get(exec, k);
        if (!element.isUndefinedOrNull()) {
            TiObject* o = element.toObject(exec);
            TiValue conversionFunction = o->get(exec, exec->propertyNames().toLocaleString);
            UString str;
            CallData callData;
            CallType callType = getCallData(conversionFunction, callData);
            if (callType != CallTypeNone)
                str = call(exec, conversionFunction, callType, callData, element, exec->emptyList()).toString(exec);
            else
                str = element.toString(exec);
            strBuffer.append(str);
        }
    }

    return TiValue::encode(strBuffer.build(exec));
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncJoin(TiExcState* exec)
{
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    StringRecursionChecker checker(exec, thisObj);
    if (EncodedTiValue earlyReturnValue = checker.earlyReturnValue())
        return earlyReturnValue;

    TiStringBuilder strBuffer;

    UString separator;
    if (!exec->argument(0).isUndefined())
        separator = exec->argument(0).toString(exec);

    unsigned k = 0;
    if (isTiArray(&exec->globalData(), thisObj)) {
        TiArray* array = asArray(thisObj);

        if (length) {
            if (!array->canGetIndex(k)) 
                goto skipFirstLoop;
            TiValue element = array->getIndex(k);
            if (!element.isUndefinedOrNull())
                strBuffer.append(element.toString(exec));
            k++;
        }

        if (separator.isNull()) {
            for (; k < length; k++) {
                if (!array->canGetIndex(k))
                    break;
                strBuffer.append(',');
                TiValue element = array->getIndex(k);
                if (!element.isUndefinedOrNull())
                    strBuffer.append(element.toString(exec));
            }
        } else {
            for (; k < length; k++) {
                if (!array->canGetIndex(k))
                    break;
                strBuffer.append(separator);
                TiValue element = array->getIndex(k);
                if (!element.isUndefinedOrNull())
                    strBuffer.append(element.toString(exec));
            }
        }
    }
 skipFirstLoop:
    for (; k < length; k++) {
        if (k >= 1) {
            if (separator.isNull())
                strBuffer.append(',');
            else
                strBuffer.append(separator);
        }

        TiValue element = thisObj->get(exec, k);
        if (!element.isUndefinedOrNull())
            strBuffer.append(element.toString(exec));
    }

    return TiValue::encode(strBuffer.build(exec));
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncConcat(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();
    TiArray* arr = constructEmptyArray(exec);
    unsigned n = 0;
    TiValue curArg = thisValue.toThisObject(exec);
    size_t i = 0;
    size_t argCount = exec->argumentCount();
    while (1) {
        if (curArg.inherits(&TiArray::s_info)) {
            unsigned length = curArg.get(exec, exec->propertyNames().length).toUInt32(exec);
            TiObject* curObject = curArg.toObject(exec);
            for (unsigned k = 0; k < length; ++k) {
                if (TiValue v = getProperty(exec, curObject, k))
                    arr->put(exec, n, v);
                n++;
            }
        } else {
            arr->put(exec, n, curArg);
            n++;
        }
        if (i == argCount)
            break;
        curArg = (exec->argument(i));
        ++i;
    }
    arr->setLength(n);
    return TiValue::encode(arr);
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncPop(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();

    if (isTiArray(&exec->globalData(), thisValue))
        return TiValue::encode(asArray(thisValue)->pop());

    TiObject* thisObj = thisValue.toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    TiValue result;
    if (length == 0) {
        putProperty(exec, thisObj, exec->propertyNames().length, jsNumber(length));
        result = jsUndefined();
    } else {
        result = thisObj->get(exec, length - 1);
        thisObj->deleteProperty(exec, length - 1);
        putProperty(exec, thisObj, exec->propertyNames().length, jsNumber(length - 1));
    }
    return TiValue::encode(result);
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncPush(TiExcState* exec)
{
    TiValue thisValue = exec->hostThisValue();

    if (isTiArray(&exec->globalData(), thisValue) && exec->argumentCount() == 1) {
        TiArray* array = asArray(thisValue);
        array->push(exec, exec->argument(0));
        return TiValue::encode(jsNumber(array->length()));
    }

    TiObject* thisObj = thisValue.toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    for (unsigned n = 0; n < exec->argumentCount(); n++)
        thisObj->put(exec, length + n, exec->argument(n));
    length += exec->argumentCount();
    putProperty(exec, thisObj, exec->propertyNames().length, jsNumber(length));
    return TiValue::encode(jsNumber(length));
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncReverse(TiExcState* exec)
{
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    unsigned middle = length / 2;
    for (unsigned k = 0; k < middle; k++) {
        unsigned lk1 = length - k - 1;
        TiValue obj2 = getProperty(exec, thisObj, lk1);
        TiValue obj = getProperty(exec, thisObj, k);

        if (obj2)
            thisObj->put(exec, k, obj2);
        else
            thisObj->deleteProperty(exec, k);

        if (obj)
            thisObj->put(exec, lk1, obj);
        else
            thisObj->deleteProperty(exec, lk1);
    }
    return TiValue::encode(thisObj);
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncShift(TiExcState* exec)
{
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    TiValue result;

    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    if (length == 0) {
        putProperty(exec, thisObj, exec->propertyNames().length, jsNumber(length));
        result = jsUndefined();
    } else {
        result = thisObj->get(exec, 0);
        if (isTiArray(&exec->globalData(), thisObj))
            ((TiArray *)thisObj)->shiftCount(exec, 1);
        else {
            for (unsigned k = 1; k < length; k++) {
                if (TiValue obj = getProperty(exec, thisObj, k))
                    thisObj->put(exec, k - 1, obj);
                else
                    thisObj->deleteProperty(exec, k - 1);
            }
            thisObj->deleteProperty(exec, length - 1);
        }
        putProperty(exec, thisObj, exec->propertyNames().length, jsNumber(length - 1));
    }
    return TiValue::encode(result);
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncSlice(TiExcState* exec)
{
    // http://developer.netscape.com/docs/manuals/js/client/jsref/array.htm#1193713 or 15.4.4.10
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);

    // We return a new array
    TiArray* resObj = constructEmptyArray(exec);
    TiValue result = resObj;

    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    unsigned begin = argumentClampedIndexFromStartOrEnd(exec, 0, length);
    unsigned end = argumentClampedIndexFromStartOrEnd(exec, 1, length, length);

    unsigned n = 0;
    for (unsigned k = begin; k < end; k++, n++) {
        if (TiValue v = getProperty(exec, thisObj, k))
            resObj->put(exec, n, v);
    }
    resObj->setLength(n);
    return TiValue::encode(result);
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncSort(TiExcState* exec)
{
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (!length || exec->hadException())
        return TiValue::encode(thisObj);

    TiValue function = exec->argument(0);
    CallData callData;
    CallType callType = getCallData(function, callData);

    if (thisObj->classInfo() == &TiArray::s_info) {
        if (isNumericCompareFunction(exec, callType, callData))
            asArray(thisObj)->sortNumeric(exec, function, callType, callData);
        else if (callType != CallTypeNone)
            asArray(thisObj)->sort(exec, function, callType, callData);
        else
            asArray(thisObj)->sort(exec);
        return TiValue::encode(thisObj);
    }

    // "Min" sort. Not the fastest, but definitely less code than heapsort
    // or quicksort, and much less swapping than bubblesort/insertionsort.
    for (unsigned i = 0; i < length - 1; ++i) {
        TiValue iObj = thisObj->get(exec, i);
        if (exec->hadException())
            return TiValue::encode(jsUndefined());
        unsigned themin = i;
        TiValue minObj = iObj;
        for (unsigned j = i + 1; j < length; ++j) {
            TiValue jObj = thisObj->get(exec, j);
            if (exec->hadException())
                return TiValue::encode(jsUndefined());
            double compareResult;
            if (jObj.isUndefined())
                compareResult = 1; // don't check minObj because there's no need to differentiate == (0) from > (1)
            else if (minObj.isUndefined())
                compareResult = -1;
            else if (callType != CallTypeNone) {
                MarkedArgumentBuffer l;
                l.append(jObj);
                l.append(minObj);
                compareResult = call(exec, function, callType, callData, exec->globalThisValue(), l).toNumber(exec);
            } else
                compareResult = (jObj.toString(exec) < minObj.toString(exec)) ? -1 : 1;

            if (compareResult < 0) {
                themin = j;
                minObj = jObj;
            }
        }
        // Swap themin and i
        if (themin > i) {
            thisObj->put(exec, i, minObj);
            thisObj->put(exec, themin, iObj);
        }
    }
    return TiValue::encode(thisObj);
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncSplice(TiExcState* exec)
{
    // 15.4.4.12

    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    if (!exec->argumentCount())
        return TiValue::encode(constructEmptyArray(exec));

    unsigned begin = argumentClampedIndexFromStartOrEnd(exec, 0, length);

    unsigned deleteCount = length - begin;
    if (exec->argumentCount() > 1) {
        double deleteDouble = exec->argument(1).toInteger(exec);
        if (deleteDouble < 0)
            deleteCount = 0;
        else if (deleteDouble > length - begin)
            deleteCount = length - begin;
        else
            deleteCount = static_cast<unsigned>(deleteDouble);
    }

    TiArray* resObj = new (exec) TiArray(exec->globalData(), exec->lexicalGlobalObject()->arrayStructure(), deleteCount, CreateCompact);
    TiValue result = resObj;
    TiGlobalData& globalData = exec->globalData();
    for (unsigned k = 0; k < deleteCount; k++)
        resObj->uncheckedSetIndex(globalData, k, getProperty(exec, thisObj, k + begin));

    resObj->setLength(deleteCount);

    unsigned additionalArgs = std::max<int>(exec->argumentCount() - 2, 0);
    if (additionalArgs != deleteCount) {
        if (additionalArgs < deleteCount) {
            if ((!begin) && (isTiArray(&exec->globalData(), thisObj)))
                ((TiArray *)thisObj)->shiftCount(exec, deleteCount - additionalArgs);
            else {
                for (unsigned k = begin; k < length - deleteCount; ++k) {
                    if (TiValue v = getProperty(exec, thisObj, k + deleteCount))
                        thisObj->put(exec, k + additionalArgs, v);
                    else
                        thisObj->deleteProperty(exec, k + additionalArgs);
                }
                for (unsigned k = length; k > length - deleteCount + additionalArgs; --k)
                    thisObj->deleteProperty(exec, k - 1);
            }
        } else {
            if ((!begin) && (isTiArray(&exec->globalData(), thisObj)))
                ((TiArray *)thisObj)->unshiftCount(exec, additionalArgs - deleteCount);
            else {
                for (unsigned k = length - deleteCount; k > begin; --k) {
                    if (TiValue obj = getProperty(exec, thisObj, k + deleteCount - 1))
                        thisObj->put(exec, k + additionalArgs - 1, obj);
                    else
                        thisObj->deleteProperty(exec, k + additionalArgs - 1);
                }
            }
        }
    }
    for (unsigned k = 0; k < additionalArgs; ++k)
        thisObj->put(exec, k + begin, exec->argument(k + 2));

    putProperty(exec, thisObj, exec->propertyNames().length, jsNumber(length - deleteCount + additionalArgs));
    return TiValue::encode(result);
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncUnShift(TiExcState* exec)
{
    // 15.4.4.13

    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    unsigned nrArgs = exec->argumentCount();
    if ((nrArgs) && (length)) {
        if (isTiArray(&exec->globalData(), thisObj))
            ((TiArray *)thisObj)->unshiftCount(exec, nrArgs);
        else {
            for (unsigned k = length; k > 0; --k) {
                if (TiValue v = getProperty(exec, thisObj, k - 1))
                    thisObj->put(exec, k + nrArgs - 1, v);
                else
                    thisObj->deleteProperty(exec, k + nrArgs - 1);
            }
        }
    }
    for (unsigned k = 0; k < nrArgs; ++k)
        thisObj->put(exec, k, exec->argument(k));
    TiValue result = jsNumber(length + nrArgs);
    putProperty(exec, thisObj, exec->propertyNames().length, result);
    return TiValue::encode(result);
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncFilter(TiExcState* exec)
{
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    TiValue function = exec->argument(0);
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return throwVMTypeError(exec);

    TiObject* applyThis = exec->argument(1).isUndefinedOrNull() ? exec->globalThisValue() : exec->argument(1).toObject(exec);
    TiArray* resultArray = constructEmptyArray(exec);

    unsigned filterIndex = 0;
    unsigned k = 0;
    if (callType == CallTypeJS && isTiArray(&exec->globalData(), thisObj)) {
        TiFunction* f = asFunction(function);
        TiArray* array = asArray(thisObj);
        CachedCall cachedCall(exec, f, 3);
        for (; k < length && !exec->hadException(); ++k) {
            if (!array->canGetIndex(k))
                break;
            TiValue v = array->getIndex(k);
            cachedCall.setThis(applyThis);
            cachedCall.setArgument(0, v);
            cachedCall.setArgument(1, jsNumber(k));
            cachedCall.setArgument(2, thisObj);
            
            TiValue result = cachedCall.call();
            if (result.toBoolean(exec))
                resultArray->put(exec, filterIndex++, v);
        }
        if (k == length)
            return TiValue::encode(resultArray);
    }
    for (; k < length && !exec->hadException(); ++k) {
        PropertySlot slot(thisObj);
        if (!thisObj->getPropertySlot(exec, k, slot))
            continue;
        TiValue v = slot.getValue(exec, k);

        if (exec->hadException())
            return TiValue::encode(jsUndefined());

        MarkedArgumentBuffer eachArguments;
        eachArguments.append(v);
        eachArguments.append(jsNumber(k));
        eachArguments.append(thisObj);

        TiValue result = call(exec, function, callType, callData, applyThis, eachArguments);
        if (result.toBoolean(exec))
            resultArray->put(exec, filterIndex++, v);
    }
    return TiValue::encode(resultArray);
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncMap(TiExcState* exec)
{
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    TiValue function = exec->argument(0);
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return throwVMTypeError(exec);

    TiObject* applyThis = exec->argument(1).isUndefinedOrNull() ? exec->globalThisValue() : exec->argument(1).toObject(exec);

    TiArray* resultArray = constructEmptyArray(exec, length);
    unsigned k = 0;
    if (callType == CallTypeJS && isTiArray(&exec->globalData(), thisObj)) {
        TiFunction* f = asFunction(function);
        TiArray* array = asArray(thisObj);
        CachedCall cachedCall(exec, f, 3);
        for (; k < length && !exec->hadException(); ++k) {
            if (UNLIKELY(!array->canGetIndex(k)))
                break;

            cachedCall.setThis(applyThis);
            cachedCall.setArgument(0, array->getIndex(k));
            cachedCall.setArgument(1, jsNumber(k));
            cachedCall.setArgument(2, thisObj);

            resultArray->TiArray::put(exec, k, cachedCall.call());
        }
    }
    for (; k < length && !exec->hadException(); ++k) {
        PropertySlot slot(thisObj);
        if (!thisObj->getPropertySlot(exec, k, slot))
            continue;
        TiValue v = slot.getValue(exec, k);

        if (exec->hadException())
            return TiValue::encode(jsUndefined());

        MarkedArgumentBuffer eachArguments;
        eachArguments.append(v);
        eachArguments.append(jsNumber(k));
        eachArguments.append(thisObj);

        if (exec->hadException())
            return TiValue::encode(jsUndefined());

        TiValue result = call(exec, function, callType, callData, applyThis, eachArguments);
        resultArray->put(exec, k, result);
    }

    return TiValue::encode(resultArray);
}

// Documentation for these three is available at:
// http://developer-test.mozilla.org/en/docs/Core_Ti_1.5_Reference:Objects:Array:every
// http://developer-test.mozilla.org/en/docs/Core_Ti_1.5_Reference:Objects:Array:forEach
// http://developer-test.mozilla.org/en/docs/Core_Ti_1.5_Reference:Objects:Array:some

EncodedTiValue JSC_HOST_CALL arrayProtoFuncEvery(TiExcState* exec)
{
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    TiValue function = exec->argument(0);
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return throwVMTypeError(exec);

    TiObject* applyThis = exec->argument(1).isUndefinedOrNull() ? exec->globalThisValue() : exec->argument(1).toObject(exec);

    TiValue result = jsBoolean(true);

    unsigned k = 0;
    if (callType == CallTypeJS && isTiArray(&exec->globalData(), thisObj)) {
        TiFunction* f = asFunction(function);
        TiArray* array = asArray(thisObj);
        CachedCall cachedCall(exec, f, 3);
        for (; k < length && !exec->hadException(); ++k) {
            if (UNLIKELY(!array->canGetIndex(k)))
                break;
            
            cachedCall.setThis(applyThis);
            cachedCall.setArgument(0, array->getIndex(k));
            cachedCall.setArgument(1, jsNumber(k));
            cachedCall.setArgument(2, thisObj);
            TiValue result = cachedCall.call();
            if (!result.toBoolean(cachedCall.newCallFrame(exec)))
                return TiValue::encode(jsBoolean(false));
        }
    }
    for (; k < length && !exec->hadException(); ++k) {
        PropertySlot slot(thisObj);
        if (!thisObj->getPropertySlot(exec, k, slot))
            continue;

        MarkedArgumentBuffer eachArguments;
        eachArguments.append(slot.getValue(exec, k));
        eachArguments.append(jsNumber(k));
        eachArguments.append(thisObj);

        if (exec->hadException())
            return TiValue::encode(jsUndefined());

        bool predicateResult = call(exec, function, callType, callData, applyThis, eachArguments).toBoolean(exec);
        if (!predicateResult) {
            result = jsBoolean(false);
            break;
        }
    }

    return TiValue::encode(result);
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncForEach(TiExcState* exec)
{
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    TiValue function = exec->argument(0);
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return throwVMTypeError(exec);

    TiObject* applyThis = exec->argument(1).isUndefinedOrNull() ? exec->globalThisValue() : exec->argument(1).toObject(exec);

    unsigned k = 0;
    if (callType == CallTypeJS && isTiArray(&exec->globalData(), thisObj)) {
        TiFunction* f = asFunction(function);
        TiArray* array = asArray(thisObj);
        CachedCall cachedCall(exec, f, 3);
        for (; k < length && !exec->hadException(); ++k) {
            if (UNLIKELY(!array->canGetIndex(k)))
                break;

            cachedCall.setThis(applyThis);
            cachedCall.setArgument(0, array->getIndex(k));
            cachedCall.setArgument(1, jsNumber(k));
            cachedCall.setArgument(2, thisObj);

            cachedCall.call();
        }
    }
    for (; k < length && !exec->hadException(); ++k) {
        PropertySlot slot(thisObj);
        if (!thisObj->getPropertySlot(exec, k, slot))
            continue;

        MarkedArgumentBuffer eachArguments;
        eachArguments.append(slot.getValue(exec, k));
        eachArguments.append(jsNumber(k));
        eachArguments.append(thisObj);

        if (exec->hadException())
            return TiValue::encode(jsUndefined());

        call(exec, function, callType, callData, applyThis, eachArguments);
    }
    return TiValue::encode(jsUndefined());
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncSome(TiExcState* exec)
{
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    TiValue function = exec->argument(0);
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return throwVMTypeError(exec);

    TiObject* applyThis = exec->argument(1).isUndefinedOrNull() ? exec->globalThisValue() : exec->argument(1).toObject(exec);

    TiValue result = jsBoolean(false);

    unsigned k = 0;
    if (callType == CallTypeJS && isTiArray(&exec->globalData(), thisObj)) {
        TiFunction* f = asFunction(function);
        TiArray* array = asArray(thisObj);
        CachedCall cachedCall(exec, f, 3);
        for (; k < length && !exec->hadException(); ++k) {
            if (UNLIKELY(!array->canGetIndex(k)))
                break;
            
            cachedCall.setThis(applyThis);
            cachedCall.setArgument(0, array->getIndex(k));
            cachedCall.setArgument(1, jsNumber(k));
            cachedCall.setArgument(2, thisObj);
            TiValue result = cachedCall.call();
            if (result.toBoolean(cachedCall.newCallFrame(exec)))
                return TiValue::encode(jsBoolean(true));
        }
    }
    for (; k < length && !exec->hadException(); ++k) {
        PropertySlot slot(thisObj);
        if (!thisObj->getPropertySlot(exec, k, slot))
            continue;

        MarkedArgumentBuffer eachArguments;
        eachArguments.append(slot.getValue(exec, k));
        eachArguments.append(jsNumber(k));
        eachArguments.append(thisObj);

        if (exec->hadException())
            return TiValue::encode(jsUndefined());

        bool predicateResult = call(exec, function, callType, callData, applyThis, eachArguments).toBoolean(exec);
        if (predicateResult) {
            result = jsBoolean(true);
            break;
        }
    }
    return TiValue::encode(result);
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncReduce(TiExcState* exec)
{
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    TiValue function = exec->argument(0);
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return throwVMTypeError(exec);

    unsigned i = 0;
    TiValue rv;
    if (!length && exec->argumentCount() == 1)
        return throwVMTypeError(exec);

    TiArray* array = 0;
    if (isTiArray(&exec->globalData(), thisObj))
        array = asArray(thisObj);

    if (exec->argumentCount() >= 2)
        rv = exec->argument(1);
    else if (array && array->canGetIndex(0)){
        rv = array->getIndex(0);
        i = 1;
    } else {
        for (i = 0; i < length; i++) {
            rv = getProperty(exec, thisObj, i);
            if (rv)
                break;
        }
        if (!rv)
            return throwVMTypeError(exec);
        i++;
    }

    if (callType == CallTypeJS && array) {
        CachedCall cachedCall(exec, asFunction(function), 4);
        for (; i < length && !exec->hadException(); ++i) {
            cachedCall.setThis(jsNull());
            cachedCall.setArgument(0, rv);
            TiValue v;
            if (LIKELY(array->canGetIndex(i)))
                v = array->getIndex(i);
            else
                break; // length has been made unsafe while we enumerate fallback to slow path
            cachedCall.setArgument(1, v);
            cachedCall.setArgument(2, jsNumber(i));
            cachedCall.setArgument(3, array);
            rv = cachedCall.call();
        }
        if (i == length) // only return if we reached the end of the array
            return TiValue::encode(rv);
    }

    for (; i < length && !exec->hadException(); ++i) {
        TiValue prop = getProperty(exec, thisObj, i);
        if (exec->hadException())
            return TiValue::encode(jsUndefined());
        if (!prop)
            continue;
        
        MarkedArgumentBuffer eachArguments;
        eachArguments.append(rv);
        eachArguments.append(prop);
        eachArguments.append(jsNumber(i));
        eachArguments.append(thisObj);
        
        rv = call(exec, function, callType, callData, jsNull(), eachArguments);
    }
    return TiValue::encode(rv);
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncReduceRight(TiExcState* exec)
{
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    TiValue function = exec->argument(0);
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return throwVMTypeError(exec);
    
    unsigned i = 0;
    TiValue rv;
    if (!length && exec->argumentCount() == 1)
        return throwVMTypeError(exec);

    TiArray* array = 0;
    if (isTiArray(&exec->globalData(), thisObj))
        array = asArray(thisObj);
    
    if (exec->argumentCount() >= 2)
        rv = exec->argument(1);
    else if (array && array->canGetIndex(length - 1)){
        rv = array->getIndex(length - 1);
        i = 1;
    } else {
        for (i = 0; i < length; i++) {
            rv = getProperty(exec, thisObj, length - i - 1);
            if (rv)
                break;
        }
        if (!rv)
            return throwVMTypeError(exec);
        i++;
    }
    
    if (callType == CallTypeJS && array) {
        CachedCall cachedCall(exec, asFunction(function), 4);
        for (; i < length && !exec->hadException(); ++i) {
            unsigned idx = length - i - 1;
            cachedCall.setThis(jsNull());
            cachedCall.setArgument(0, rv);
            if (UNLIKELY(!array->canGetIndex(idx)))
                break; // length has been made unsafe while we enumerate fallback to slow path
            cachedCall.setArgument(1, array->getIndex(idx));
            cachedCall.setArgument(2, jsNumber(idx));
            cachedCall.setArgument(3, array);
            rv = cachedCall.call();
        }
        if (i == length) // only return if we reached the end of the array
            return TiValue::encode(rv);
    }
    
    for (; i < length && !exec->hadException(); ++i) {
        unsigned idx = length - i - 1;
        TiValue prop = getProperty(exec, thisObj, idx);
        if (exec->hadException())
            return TiValue::encode(jsUndefined());
        if (!prop)
            continue;
        
        MarkedArgumentBuffer eachArguments;
        eachArguments.append(rv);
        eachArguments.append(prop);
        eachArguments.append(jsNumber(idx));
        eachArguments.append(thisObj);
        
        rv = call(exec, function, callType, callData, jsNull(), eachArguments);
    }
    return TiValue::encode(rv);        
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncIndexOf(TiExcState* exec)
{
    // 15.4.4.14
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    unsigned index = argumentClampedIndexFromStartOrEnd(exec, 1, length);
    TiValue searchElement = exec->argument(0);
    for (; index < length; ++index) {
        TiValue e = getProperty(exec, thisObj, index);
        if (!e)
            continue;
        if (TiValue::strictEqual(exec, searchElement, e))
            return TiValue::encode(jsNumber(index));
    }

    return TiValue::encode(jsNumber(-1));
}

EncodedTiValue JSC_HOST_CALL arrayProtoFuncLastIndexOf(TiExcState* exec)
{
    // 15.4.4.15
    TiObject* thisObj = exec->hostThisValue().toThisObject(exec);
    unsigned length = thisObj->get(exec, exec->propertyNames().length).toUInt32(exec);
    if (!length)
        return TiValue::encode(jsNumber(-1));

    unsigned index = length - 1;
    TiValue fromValue = exec->argument(1);
    if (!fromValue.isUndefined()) {
        double fromDouble = fromValue.toInteger(exec);
        if (fromDouble < 0) {
            fromDouble += length;
            if (fromDouble < 0)
                return TiValue::encode(jsNumber(-1));
        }
        if (fromDouble < length)
            index = static_cast<unsigned>(fromDouble);
    }

    TiValue searchElement = exec->argument(0);
    do {
        ASSERT(index < length);
        TiValue e = getProperty(exec, thisObj, index);
        if (!e)
            continue;
        if (TiValue::strictEqual(exec, searchElement, e))
            return TiValue::encode(jsNumber(index));
    } while (index--);

    return TiValue::encode(jsNumber(-1));
}

} // namespace TI
