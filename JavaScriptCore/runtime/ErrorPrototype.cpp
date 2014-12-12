/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2008 Apple Inc. All rights reserved.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "ErrorPrototype.h"

#include "Error.h"
#include "JSFunction.h"
#include "JSString.h"
#include "JSStringBuilder.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "StringRecursionChecker.h"

namespace TI {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(ErrorPrototype);

static EncodedTiValue JSC_HOST_CALL errorProtoFuncToString(ExecState*);

}

#include "ErrorPrototype.lut.h"

namespace TI {

const ClassInfo ErrorPrototype::s_info = { "Error", &ErrorInstance::s_info, 0, ExecState::errorPrototypeTable, CREATE_METHOD_TABLE(ErrorPrototype) };

/* Source for ErrorPrototype.lut.h
@begin errorPrototypeTable
  toString          errorProtoFuncToString         DontEnum|Function 0
@end
*/

ErrorPrototype::ErrorPrototype(VM& vm, Structure* structure)
    : ErrorInstance(vm, structure)
{
}

void ErrorPrototype::finishCreation(VM& vm, JSGlobalObject*)
{
    Base::finishCreation(vm, "");
    ASSERT(inherits(info()));
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(&vm, String(ASCIILiteral("Error"))), DontEnum);
}

bool ErrorPrototype::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<ErrorInstance>(exec, ExecState::errorPrototypeTable(exec), jsCast<ErrorPrototype*>(object), propertyName, slot);
}

// ------------------------------ Functions ---------------------------

// ECMA-262 5.1, 15.11.4.4
EncodedTiValue JSC_HOST_CALL errorProtoFuncToString(ExecState* exec)
{
    // 1. Let O be the this value.
    TiValue thisValue = exec->hostThisValue();

    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisValue.isObject())
        return throwVMTypeError(exec);
    JSObject* thisObj = asObject(thisValue);

    // Guard against recursion!
    StringRecursionChecker checker(exec, thisObj);
    if (TiValue earlyReturnValue = checker.earlyReturnValue())
        return TiValue::encode(earlyReturnValue);

    // 3. Let name be the result of calling the [[Get]] internal method of O with argument "name".
    TiValue name = thisObj->get(exec, exec->propertyNames().name);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    // 4. If name is undefined, then let name be "Error"; else let name be ToString(name).
    String nameString;
    if (name.isUndefined())
        nameString = ASCIILiteral("Error");
    else {
        nameString = name.toString(exec)->value(exec);
        if (exec->hadException())
            return TiValue::encode(jsUndefined());
    }

    // 5. Let msg be the result of calling the [[Get]] internal method of O with argument "message".
    TiValue message = thisObj->get(exec, exec->propertyNames().message);
    if (exec->hadException())
        return TiValue::encode(jsUndefined());

    // (sic)
    // 6. If msg is undefined, then let msg be the empty String; else let msg be ToString(msg).
    // 7. If msg is undefined, then let msg be the empty String; else let msg be ToString(msg).
    String messageString;
    if (message.isUndefined())
        messageString = String();
    else {
        messageString = message.toString(exec)->value(exec);
        if (exec->hadException())
            return TiValue::encode(jsUndefined());
    }

    // 8. If name is the empty String, return msg.
    if (!nameString.length())
        return TiValue::encode(message.isString() ? message : jsString(exec, messageString));

    // 9. If msg is the empty String, return name.
    if (!messageString.length())
        return TiValue::encode(name.isString() ? name : jsNontrivialString(exec, nameString));

    // 10. Return the result of concatenating name, ":", a single space character, and msg.
    return TiValue::encode(jsMakeNontrivialString(exec, nameString, ": ", messageString));
}

} // namespace TI
