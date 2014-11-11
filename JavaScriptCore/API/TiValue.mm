/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#import "APICast.h"
#import "APIShims.h"
#import "DateInstance.h"
#import "Error.h"
#import "TiCore.h"
#import "TiContextInternal.h"
#import "TiVirtualMachineInternal.h"
#import "TiValueInternal.h"
#import "JSWrapperMap.h"
#import "ObjcRuntimeExtras.h"
#import "Operations.h"
#import "JSCTiValue.h"
#import <wtf/HashMap.h>
#import <wtf/HashSet.h>
#import <wtf/ObjcRuntimeExtras.h>
#import <wtf/Vector.h>
#import <wtf/TCSpinLock.h>
#import <wtf/text/WTFString.h>
#import <wtf/text/StringHash.h>

#if JSC_OBJC_API_ENABLED

NSString * const JSPropertyDescriptorWritableKey = @"writable";
NSString * const JSPropertyDescriptorEnumerableKey = @"enumerable";
NSString * const JSPropertyDescriptorConfigurableKey = @"configurable";
NSString * const JSPropertyDescriptorValueKey = @"value";
NSString * const JSPropertyDescriptorGetKey = @"get";
NSString * const JSPropertyDescriptorSetKey = @"set";

@implementation TiValue {
    TiValueRef m_value;
}

- (TiValueRef)TiValueRef
{
    return m_value;
}

+ (TiValue *)valueWithObject:(id)value inContext:(TiContext *)context
{
    return [TiValue valueWithTiValueRef:objectToValue(context, value) inContext:context];
}

+ (TiValue *)valueWithBool:(BOOL)value inContext:(TiContext *)context
{
    return [TiValue valueWithTiValueRef:TiValueMakeBoolean([context TiGlobalContextRef], value) inContext:context];
}

+ (TiValue *)valueWithDouble:(double)value inContext:(TiContext *)context
{
    return [TiValue valueWithTiValueRef:TiValueMakeNumber([context TiGlobalContextRef], value) inContext:context];
}

+ (TiValue *)valueWithInt32:(int32_t)value inContext:(TiContext *)context
{
    return [TiValue valueWithTiValueRef:TiValueMakeNumber([context TiGlobalContextRef], value) inContext:context];
}

+ (TiValue *)valueWithUInt32:(uint32_t)value inContext:(TiContext *)context
{
    return [TiValue valueWithTiValueRef:TiValueMakeNumber([context TiGlobalContextRef], value) inContext:context];
}

+ (TiValue *)valueWithNewObjectInContext:(TiContext *)context
{
    return [TiValue valueWithTiValueRef:TiObjectMake([context TiGlobalContextRef], 0, 0) inContext:context];
}

+ (TiValue *)valueWithNewArrayInContext:(TiContext *)context
{
    return [TiValue valueWithTiValueRef:TiObjectMakeArray([context TiGlobalContextRef], 0, NULL, 0) inContext:context];
}

+ (TiValue *)valueWithNewRegularExpressionFromPattern:(NSString *)pattern flags:(NSString *)flags inContext:(TiContext *)context
{
    TiStringRef patternString = TiStringCreateWithCFString((CFStringRef)pattern);
    TiStringRef flagsString = TiStringCreateWithCFString((CFStringRef)flags);
    TiValueRef arguments[2] = { TiValueMakeString([context TiGlobalContextRef], patternString), TiValueMakeString([context TiGlobalContextRef], flagsString) };
    TiStringRelease(patternString);
    TiStringRelease(flagsString);

    return [TiValue valueWithTiValueRef:TiObjectMakeRegExp([context TiGlobalContextRef], 2, arguments, 0) inContext:context];
}

+ (TiValue *)valueWithNewErrorFromMessage:(NSString *)message inContext:(TiContext *)context
{
    TiStringRef string = TiStringCreateWithCFString((CFStringRef)message);
    TiValueRef argument = TiValueMakeString([context TiGlobalContextRef], string);
    TiStringRelease(string);

    return [TiValue valueWithTiValueRef:TiObjectMakeError([context TiGlobalContextRef], 1, &argument, 0) inContext:context];
}

+ (TiValue *)valueWithNullInContext:(TiContext *)context
{
    return [TiValue valueWithTiValueRef:TiValueMakeNull([context TiGlobalContextRef]) inContext:context];
}

+ (TiValue *)valueWithUndefinedInContext:(TiContext *)context
{
    return [TiValue valueWithTiValueRef:TiValueMakeUndefined([context TiGlobalContextRef]) inContext:context];
}

- (id)toObject
{
    return valueToObject(_context, m_value);
}

- (id)toObjectOfClass:(Class)expectedClass
{
    id result = [self toObject];
    return [result isKindOfClass:expectedClass] ? result : nil;
}

- (BOOL)toBool
{
    return TiValueToBoolean([_context TiGlobalContextRef], m_value);
}

- (double)toDouble
{
    TiValueRef exception = 0;
    double result = TiValueToNumber([_context TiGlobalContextRef], m_value, &exception);
    if (exception) {
        [_context notifyException:exception];
        return std::numeric_limits<double>::quiet_NaN();
    }

    return result;
}

- (int32_t)toInt32
{
    return TI::toInt32([self toDouble]);
}

- (uint32_t)toUInt32
{
    return TI::toUInt32([self toDouble]);
}

- (NSNumber *)toNumber
{
    TiValueRef exception = 0;
    id result = valueToNumber([_context TiGlobalContextRef], m_value, &exception);
    if (exception)
        [_context notifyException:exception];
    return result;
}

- (NSString *)toString
{
    TiValueRef exception = 0;
    id result = valueToString([_context TiGlobalContextRef], m_value, &exception);
    if (exception)
        [_context notifyException:exception];
    return result;
}

- (NSDate *)toDate
{
    TiValueRef exception = 0;
    id result = valueToDate([_context TiGlobalContextRef], m_value, &exception);
    if (exception)
        [_context notifyException:exception];
    return result;
}

- (NSArray *)toArray
{
    TiValueRef exception = 0;
    id result = valueToArray([_context TiGlobalContextRef], m_value, &exception);
    if (exception)
        [_context notifyException:exception];
    return result;
}

- (NSDictionary *)toDictionary
{
    TiValueRef exception = 0;
    id result = valueToDictionary([_context TiGlobalContextRef], m_value, &exception);
    if (exception)
        [_context notifyException:exception];
    return result;
}

- (TiValue *)valueForProperty:(NSString *)propertyName
{
    TiValueRef exception = 0;
    TiObjectRef object = TiValueToObject([_context TiGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    TiStringRef name = TiStringCreateWithCFString((CFStringRef)propertyName);
    TiValueRef result = TiObjectGetProperty([_context TiGlobalContextRef], object, name, &exception);
    TiStringRelease(name);
    if (exception)
        return [_context valueFromNotifyException:exception];

    return [TiValue valueWithTiValueRef:result inContext:_context];
}

- (void)setValue:(id)value forProperty:(NSString *)propertyName
{
    TiValueRef exception = 0;
    TiObjectRef object = TiValueToObject([_context TiGlobalContextRef], m_value, &exception);
    if (exception) {
        [_context notifyException:exception];
        return;
    }

    TiStringRef name = TiStringCreateWithCFString((CFStringRef)propertyName);
    TiObjectSetProperty([_context TiGlobalContextRef], object, name, objectToValue(_context, value), 0, &exception);
    TiStringRelease(name);
    if (exception) {
        [_context notifyException:exception];
        return;
    }
}

- (BOOL)deleteProperty:(NSString *)propertyName
{
    TiValueRef exception = 0;
    TiObjectRef object = TiValueToObject([_context TiGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context boolFromNotifyException:exception];

    TiStringRef name = TiStringCreateWithCFString((CFStringRef)propertyName);
    BOOL result = TiObjectDeleteProperty([_context TiGlobalContextRef], object, name, &exception);
    TiStringRelease(name);
    if (exception)
        return [_context boolFromNotifyException:exception];

    return result;
}

- (BOOL)hasProperty:(NSString *)propertyName
{
    TiValueRef exception = 0;
    TiObjectRef object = TiValueToObject([_context TiGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context boolFromNotifyException:exception];

    TiStringRef name = TiStringCreateWithCFString((CFStringRef)propertyName);
    BOOL result = TiObjectHasProperty([_context TiGlobalContextRef], object, name);
    TiStringRelease(name);
    return result;
}

- (void)defineProperty:(NSString *)property descriptor:(id)descriptor
{
    [[_context globalObject][@"Object"] invokeMethod:@"defineProperty" withArguments:@[ self, property, descriptor ]];
}

- (TiValue *)valueAtIndex:(NSUInteger)index
{
    // Properties that are higher than an unsigned value can hold are converted to a double then inserted as a normal property.
    // Indices that are bigger than the max allowed index size (UINT_MAX - 1) will be handled internally in get().
    if (index != (unsigned)index)
        return [self valueForProperty:[[TiValue valueWithDouble:index inContext:_context] toString]];

    TiValueRef exception = 0;
    TiObjectRef object = TiValueToObject([_context TiGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    TiValueRef result = TiObjectGetPropertyAtIndex([_context TiGlobalContextRef], object, (unsigned)index, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    return [TiValue valueWithTiValueRef:result inContext:_context];
}

- (void)setValue:(id)value atIndex:(NSUInteger)index
{
    // Properties that are higher than an unsigned value can hold are converted to a double, then inserted as a normal property.
    // Indices that are bigger than the max allowed index size (UINT_MAX - 1) will be handled internally in putByIndex().
    if (index != (unsigned)index)
        return [self setValue:value forProperty:[[TiValue valueWithDouble:index inContext:_context] toString]];

    TiValueRef exception = 0;
    TiObjectRef object = TiValueToObject([_context TiGlobalContextRef], m_value, &exception);
    if (exception) {
        [_context notifyException:exception];
        return;
    }

    TiObjectSetPropertyAtIndex([_context TiGlobalContextRef], object, (unsigned)index, objectToValue(_context, value), &exception);
    if (exception) {
        [_context notifyException:exception];
        return;
    }
}

- (BOOL)isUndefined
{
    return TiValueIsUndefined([_context TiGlobalContextRef], m_value);
}

- (BOOL)isNull
{
    return TiValueIsNull([_context TiGlobalContextRef], m_value);
}

- (BOOL)isBoolean
{
    return TiValueIsBoolean([_context TiGlobalContextRef], m_value);
}

- (BOOL)isNumber
{
    return TiValueIsNumber([_context TiGlobalContextRef], m_value);
}

- (BOOL)isString
{
    return TiValueIsString([_context TiGlobalContextRef], m_value);
}

- (BOOL)isObject
{
    return TiValueIsObject([_context TiGlobalContextRef], m_value);
}

- (BOOL)isEqualToObject:(id)value
{
    return TiValueIsStrictEqual([_context TiGlobalContextRef], m_value, objectToValue(_context, value));
}

- (BOOL)isEqualWithTypeCoercionToObject:(id)value
{
    TiValueRef exception = 0;
    BOOL result = TiValueIsEqual([_context TiGlobalContextRef], m_value, objectToValue(_context, value), &exception);
    if (exception)
        return [_context boolFromNotifyException:exception];

    return result;
}

- (BOOL)isInstanceOf:(id)value
{
    TiValueRef exception = 0;
    TiObjectRef constructor = TiValueToObject([_context TiGlobalContextRef], objectToValue(_context, value), &exception);
    if (exception)
        return [_context boolFromNotifyException:exception];

    BOOL result = TiValueIsInstanceOfConstructor([_context TiGlobalContextRef], m_value, constructor, &exception);
    if (exception)
        return [_context boolFromNotifyException:exception];

    return result;
}

- (TiValue *)callWithArguments:(NSArray *)argumentArray
{
    NSUInteger argumentCount = [argumentArray count];
    TiValueRef arguments[argumentCount];
    for (unsigned i = 0; i < argumentCount; ++i)
        arguments[i] = objectToValue(_context, [argumentArray objectAtIndex:i]);

    TiValueRef exception = 0;
    TiObjectRef object = TiValueToObject([_context TiGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    TiValueRef result = TiObjectCallAsFunction([_context TiGlobalContextRef], object, 0, argumentCount, arguments, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    return [TiValue valueWithTiValueRef:result inContext:_context];
}

- (TiValue *)constructWithArguments:(NSArray *)argumentArray
{
    NSUInteger argumentCount = [argumentArray count];
    TiValueRef arguments[argumentCount];
    for (unsigned i = 0; i < argumentCount; ++i)
        arguments[i] = objectToValue(_context, [argumentArray objectAtIndex:i]);

    TiValueRef exception = 0;
    TiObjectRef object = TiValueToObject([_context TiGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    TiObjectRef result = TiObjectCallAsConstructor([_context TiGlobalContextRef], object, argumentCount, arguments, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    return [TiValue valueWithTiValueRef:result inContext:_context];
}

- (TiValue *)invokeMethod:(NSString *)method withArguments:(NSArray *)arguments
{
    NSUInteger argumentCount = [arguments count];
    TiValueRef argumentArray[argumentCount];
    for (unsigned i = 0; i < argumentCount; ++i)
        argumentArray[i] = objectToValue(_context, [arguments objectAtIndex:i]);

    TiValueRef exception = 0;
    TiObjectRef thisObject = TiValueToObject([_context TiGlobalContextRef], m_value, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    TiStringRef name = TiStringCreateWithCFString((CFStringRef)method);
    TiValueRef function = TiObjectGetProperty([_context TiGlobalContextRef], thisObject, name, &exception);
    TiStringRelease(name);
    if (exception)
        return [_context valueFromNotifyException:exception];

    TiObjectRef object = TiValueToObject([_context TiGlobalContextRef], function, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    TiValueRef result = TiObjectCallAsFunction([_context TiGlobalContextRef], object, thisObject, argumentCount, argumentArray, &exception);
    if (exception)
        return [_context valueFromNotifyException:exception];

    return [TiValue valueWithTiValueRef:result inContext:_context];
}

@end

@implementation TiValue(StructSupport)

- (CGPoint)toPoint
{
    return (CGPoint){
        static_cast<CGFloat>([self[@"x"] toDouble]),
        static_cast<CGFloat>([self[@"y"] toDouble])
    };
}

- (NSRange)toRange
{
    return (NSRange){
        [[self[@"location"] toNumber] unsignedIntegerValue],
        [[self[@"length"] toNumber] unsignedIntegerValue]
    };
}

- (CGRect)toRect
{
    return (CGRect){
        [self toPoint],
        [self toSize]
    };
}

- (CGSize)toSize
{
    return (CGSize){
        static_cast<CGFloat>([self[@"width"] toDouble]),
        static_cast<CGFloat>([self[@"height"] toDouble])
    };
}

+ (TiValue *)valueWithPoint:(CGPoint)point inContext:(TiContext *)context
{
    return [TiValue valueWithObject:@{
        @"x":@(point.x),
        @"y":@(point.y)
    } inContext:context];
}

+ (TiValue *)valueWithRange:(NSRange)range inContext:(TiContext *)context
{
    return [TiValue valueWithObject:@{
        @"location":@(range.location),
        @"length":@(range.length)
    } inContext:context];
}

+ (TiValue *)valueWithRect:(CGRect)rect inContext:(TiContext *)context
{
    return [TiValue valueWithObject:@{
        @"x":@(rect.origin.x),
        @"y":@(rect.origin.y),
        @"width":@(rect.size.width),
        @"height":@(rect.size.height)
    } inContext:context];
}

+ (TiValue *)valueWithSize:(CGSize)size inContext:(TiContext *)context
{
    return [TiValue valueWithObject:@{
        @"width":@(size.width),
        @"height":@(size.height)
    } inContext:context];
}

@end

@implementation TiValue(SubscriptSupport)

- (TiValue *)objectForKeyedSubscript:(id)key
{
    if (![key isKindOfClass:[NSString class]]) {
        key = [[TiValue valueWithObject:key inContext:_context] toString];
        if (!key)
            return [TiValue valueWithUndefinedInContext:_context];
    }

    return [self valueForProperty:(NSString *)key];
}

- (TiValue *)objectAtIndexedSubscript:(NSUInteger)index
{
    return [self valueAtIndex:index];
}

- (void)setObject:(id)object forKeyedSubscript:(NSObject <NSCopying> *)key
{
    if (![key isKindOfClass:[NSString class]]) {
        key = [[TiValue valueWithObject:key inContext:_context] toString];
        if (!key)
            return;
    }

    [self setValue:object forProperty:(NSString *)key];
}

- (void)setObject:(id)object atIndexedSubscript:(NSUInteger)index
{
    [self setValue:object atIndex:index];
}

@end

inline bool isDate(TiObjectRef object, TiGlobalContextRef context)
{
    TI::APIEntryShim entryShim(toJS(context));
    return toJS(object)->inherits(TI::DateInstance::info());
}

inline bool isArray(TiObjectRef object, TiGlobalContextRef context)
{
    TI::APIEntryShim entryShim(toJS(context));
    return toJS(object)->inherits(TI::JSArray::info());
}

@implementation TiValue(Internal)

enum ConversionType {
    ContainerNone,
    ContainerArray,
    ContainerDictionary
};

class JSContainerConvertor {
public:
    struct Task {
        TiValueRef js;
        id objc;
        ConversionType type;
    };

    JSContainerConvertor(TiGlobalContextRef context)
        : m_context(context)
    {
    }

    id convert(TiValueRef property);
    void add(Task);
    Task take();
    bool isWorkListEmpty() const { return !m_worklist.size(); }

private:
    TiGlobalContextRef m_context;
    HashMap<TiValueRef, id> m_objectMap;
    Vector<Task> m_worklist;
};

inline id JSContainerConvertor::convert(TiValueRef value)
{
    HashMap<TiValueRef, id>::iterator iter = m_objectMap.find(value);
    if (iter != m_objectMap.end())
        return iter->value;

    Task result = valueToObjectWithoutCopy(m_context, value);
    if (result.js)
        add(result);
    return result.objc;
}

void JSContainerConvertor::add(Task task)
{
    m_objectMap.add(task.js, task.objc);
    if (task.type != ContainerNone)
        m_worklist.append(task);
}

JSContainerConvertor::Task JSContainerConvertor::take()
{
    ASSERT(!isWorkListEmpty());
    Task last = m_worklist.last();
    m_worklist.removeLast();
    return last;
}

static JSContainerConvertor::Task valueToObjectWithoutCopy(TiGlobalContextRef context, TiValueRef value)
{
    if (!TiValueIsObject(context, value)) {
        id primitive;
        if (TiValueIsBoolean(context, value))
            primitive = TiValueToBoolean(context, value) ? @YES : @NO;
        else if (TiValueIsNumber(context, value)) {
            // Normalize the number, so it will unique correctly in the hash map -
            // it's nicer not to leak this internal implementation detail!
            value = TiValueMakeNumber(context, TiValueToNumber(context, value, 0));
            primitive = [NSNumber numberWithDouble:TiValueToNumber(context, value, 0)];
        } else if (TiValueIsString(context, value)) {
            // Would be nice to unique strings, too.
            TiStringRef jsstring = TiValueToStringCopy(context, value, 0);
            NSString * stringNS = (NSString *)TiStringCopyCFString(kCFAllocatorDefault, jsstring);
            TiStringRelease(jsstring);
            primitive = [stringNS autorelease];
        } else if (TiValueIsNull(context, value))
            primitive = [NSNull null];
        else {
            ASSERT(TiValueIsUndefined(context, value));
            primitive = nil;
        }
        return (JSContainerConvertor::Task){ value, primitive, ContainerNone };
    }

    TiObjectRef object = TiValueToObject(context, value, 0);

    if (id wrapped = tryUnwrapObjcObject(context, object))
        return (JSContainerConvertor::Task){ object, wrapped, ContainerNone };

    if (isDate(object, context))
        return (JSContainerConvertor::Task){ object, [NSDate dateWithTimeIntervalSince1970:TiValueToNumber(context, object, 0)], ContainerNone };

    if (isArray(object, context))
        return (JSContainerConvertor::Task){ object, [NSMutableArray array], ContainerArray };

    return (JSContainerConvertor::Task){ object, [NSMutableDictionary dictionary], ContainerDictionary };
}

static id containerValueToObject(TiGlobalContextRef context, JSContainerConvertor::Task task)
{
    ASSERT(task.type != ContainerNone);
    JSContainerConvertor convertor(context);
    convertor.add(task);
    ASSERT(!convertor.isWorkListEmpty());
    
    do {
        JSContainerConvertor::Task current = convertor.take();
        ASSERT(TiValueIsObject(context, current.js));
        TiObjectRef js = TiValueToObject(context, current.js, 0);

        if (current.type == ContainerArray) {
            ASSERT([current.objc isKindOfClass:[NSMutableArray class]]);
            NSMutableArray *array = (NSMutableArray *)current.objc;
        
            TiStringRef lengthString = TiStringCreateWithUTF8CString("length");
            unsigned length = TI::toUInt32(TiValueToNumber(context, TiObjectGetProperty(context, js, lengthString, 0), 0));
            TiStringRelease(lengthString);

            for (unsigned i = 0; i < length; ++i) {
                id objc = convertor.convert(TiObjectGetPropertyAtIndex(context, js, i, 0));
                [array addObject:objc ? objc : [NSNull null]];
            }
        } else {
            ASSERT([current.objc isKindOfClass:[NSMutableDictionary class]]);
            NSMutableDictionary *dictionary = (NSMutableDictionary *)current.objc;

            TiPropertyNameArrayRef propertyNameArray = TiObjectCopyPropertyNames(context, js);
            size_t length = TiPropertyNameArrayGetCount(propertyNameArray);

            for (size_t i = 0; i < length; ++i) {
                TiStringRef propertyName = TiPropertyNameArrayGetNameAtIndex(propertyNameArray, i);
                if (id objc = convertor.convert(TiObjectGetProperty(context, js, propertyName, 0)))
                    dictionary[[(NSString *)TiStringCopyCFString(kCFAllocatorDefault, propertyName) autorelease]] = objc;
            }

            TiPropertyNameArrayRelease(propertyNameArray);
        }

    } while (!convertor.isWorkListEmpty());

    return task.objc;
}

id valueToObject(TiContext *context, TiValueRef value)
{
    JSContainerConvertor::Task result = valueToObjectWithoutCopy([context TiGlobalContextRef], value);
    if (result.type == ContainerNone)
        return result.objc;
    return containerValueToObject([context TiGlobalContextRef], result);
}

id valueToNumber(TiGlobalContextRef context, TiValueRef value, TiValueRef* exception)
{
    ASSERT(!*exception);
    if (id wrapped = tryUnwrapObjcObject(context, value)) {
        if ([wrapped isKindOfClass:[NSNumber class]])
            return wrapped;
    }

    if (TiValueIsBoolean(context, value))
        return TiValueToBoolean(context, value) ? @YES : @NO;

    double result = TiValueToNumber(context, value, exception);
    return [NSNumber numberWithDouble:*exception ? std::numeric_limits<double>::quiet_NaN() : result];
}

id valueToString(TiGlobalContextRef context, TiValueRef value, TiValueRef* exception)
{
    ASSERT(!*exception);
    if (id wrapped = tryUnwrapObjcObject(context, value)) {
        if ([wrapped isKindOfClass:[NSString class]])
            return wrapped;
    }

    TiStringRef jsstring = TiValueToStringCopy(context, value, exception);
    if (*exception) {
        ASSERT(!jsstring);
        return nil;
    }

    NSString *stringNS = CFBridgingRelease(TiStringCopyCFString(kCFAllocatorDefault, jsstring));
    TiStringRelease(jsstring);
    return stringNS;
}

id valueToDate(TiGlobalContextRef context, TiValueRef value, TiValueRef* exception)
{
    ASSERT(!*exception);
    if (id wrapped = tryUnwrapObjcObject(context, value)) {
        if ([wrapped isKindOfClass:[NSDate class]])
            return wrapped;
    }

    double result = TiValueToNumber(context, value, exception);
    return *exception ? nil : [NSDate dateWithTimeIntervalSince1970:result];
}

id valueToArray(TiGlobalContextRef context, TiValueRef value, TiValueRef* exception)
{
    ASSERT(!*exception);
    if (id wrapped = tryUnwrapObjcObject(context, value)) {
        if ([wrapped isKindOfClass:[NSArray class]])
            return wrapped;
    }

    if (TiValueIsObject(context, value))
        return containerValueToObject(context, (JSContainerConvertor::Task){ value, [NSMutableArray array], ContainerArray});

    if (!(TiValueIsNull(context, value) || TiValueIsUndefined(context, value)))
        *exception = toRef(TI::createTypeError(toJS(context), "Cannot convert primitive to NSArray"));
    return nil;
}

id valueToDictionary(TiGlobalContextRef context, TiValueRef value, TiValueRef* exception)
{
    ASSERT(!*exception);
    if (id wrapped = tryUnwrapObjcObject(context, value)) {
        if ([wrapped isKindOfClass:[NSDictionary class]])
            return wrapped;
    }

    if (TiValueIsObject(context, value))
        return containerValueToObject(context, (JSContainerConvertor::Task){ value, [NSMutableDictionary dictionary], ContainerDictionary});

    if (!(TiValueIsNull(context, value) || TiValueIsUndefined(context, value)))
        *exception = toRef(TI::createTypeError(toJS(context), "Cannot convert primitive to NSDictionary"));
    return nil;
}

class ObjcContainerConvertor {
public:
    struct Task {
        id objc;
        TiValueRef js;
        ConversionType type;
    };

    ObjcContainerConvertor(TiContext *context)
        : m_context(context)
    {
    }

    TiValueRef convert(id object);
    void add(Task);
    Task take();
    bool isWorkListEmpty() const { return !m_worklist.size(); }

private:
    TiContext *m_context;
    HashMap<id, TiValueRef> m_objectMap;
    Vector<Task> m_worklist;
};

TiValueRef ObjcContainerConvertor::convert(id object)
{
    ASSERT(object);

    auto it = m_objectMap.find(object);
    if (it != m_objectMap.end())
        return it->value;

    ObjcContainerConvertor::Task task = objectToValueWithoutCopy(m_context, object);
    add(task);
    return task.js;
}

void ObjcContainerConvertor::add(ObjcContainerConvertor::Task task)
{
    m_objectMap.add(task.objc, task.js);
    if (task.type != ContainerNone)
        m_worklist.append(task);
}

ObjcContainerConvertor::Task ObjcContainerConvertor::take()
{
    ASSERT(!isWorkListEmpty());
    Task last = m_worklist.last();
    m_worklist.removeLast();
    return last;
}

inline bool isNSBoolean(id object)
{
    ASSERT([@YES class] == [@NO class]);
    ASSERT([@YES class] != [NSNumber class]);
    ASSERT([[@YES class] isSubclassOfClass:[NSNumber class]]);
    return [object isKindOfClass:[@YES class]];
}

static ObjcContainerConvertor::Task objectToValueWithoutCopy(TiContext *context, id object)
{
    TiGlobalContextRef contextRef = [context TiGlobalContextRef];

    if (!object)
        return (ObjcContainerConvertor::Task){ object, TiValueMakeUndefined(contextRef), ContainerNone };

    if (!class_conformsToProtocol(object_getClass(object), getTiExportProtocol())) {
        if ([object isKindOfClass:[NSArray class]])
            return (ObjcContainerConvertor::Task){ object, TiObjectMakeArray(contextRef, 0, NULL, 0), ContainerArray };

        if ([object isKindOfClass:[NSDictionary class]])
            return (ObjcContainerConvertor::Task){ object, TiObjectMake(contextRef, 0, 0), ContainerDictionary };

        if ([object isKindOfClass:[NSNull class]])
            return (ObjcContainerConvertor::Task){ object, TiValueMakeNull(contextRef), ContainerNone };

        if ([object isKindOfClass:[TiValue class]])
            return (ObjcContainerConvertor::Task){ object, ((TiValue *)object)->m_value, ContainerNone };

        if ([object isKindOfClass:[NSString class]]) {
            TiStringRef string = TiStringCreateWithCFString((CFStringRef)object);
            TiValueRef js = TiValueMakeString(contextRef, string);
            TiStringRelease(string);
            return (ObjcContainerConvertor::Task){ object, js, ContainerNone };
        }

        if ([object isKindOfClass:[NSNumber class]]) {
            if (isNSBoolean(object))
                return (ObjcContainerConvertor::Task){ object, TiValueMakeBoolean(contextRef, [object boolValue]), ContainerNone };
            return (ObjcContainerConvertor::Task){ object, TiValueMakeNumber(contextRef, [object doubleValue]), ContainerNone };
        }

        if ([object isKindOfClass:[NSDate class]]) {
            TiValueRef argument = TiValueMakeNumber(contextRef, [object timeIntervalSince1970]);
            TiObjectRef result = TiObjectMakeDate(contextRef, 1, &argument, 0);
            return (ObjcContainerConvertor::Task){ object, result, ContainerNone };
        }

        if ([object isKindOfClass:[TiManagedValue class]]) {
            TiValue *value = [static_cast<TiManagedValue *>(object) value];
            if (!value)
                return (ObjcContainerConvertor::Task) { object, TiValueMakeUndefined(contextRef), ContainerNone };
            return (ObjcContainerConvertor::Task){ object, value->m_value, ContainerNone };
        }
    }

    return (ObjcContainerConvertor::Task){ object, valueInternalValue([context wrapperForObjCObject:object]), ContainerNone };
}

TiValueRef objectToValue(TiContext *context, id object)
{
    TiGlobalContextRef contextRef = [context TiGlobalContextRef];

    ObjcContainerConvertor::Task task = objectToValueWithoutCopy(context, object);
    if (task.type == ContainerNone)
        return task.js;

    ObjcContainerConvertor convertor(context);
    convertor.add(task);
    ASSERT(!convertor.isWorkListEmpty());

    do {
        ObjcContainerConvertor::Task current = convertor.take();
        ASSERT(TiValueIsObject(contextRef, current.js));
        TiObjectRef js = TiValueToObject(contextRef, current.js, 0);

        if (current.type == ContainerArray) {
            ASSERT([current.objc isKindOfClass:[NSArray class]]);
            NSArray *array = (NSArray *)current.objc;
            NSUInteger count = [array count];
            for (NSUInteger index = 0; index < count; ++index)
                TiObjectSetPropertyAtIndex(contextRef, js, index, convertor.convert([array objectAtIndex:index]), 0);
        } else {
            ASSERT(current.type == ContainerDictionary);
            ASSERT([current.objc isKindOfClass:[NSDictionary class]]);
            NSDictionary *dictionary = (NSDictionary *)current.objc;
            for (id key in [dictionary keyEnumerator]) {
                if ([key isKindOfClass:[NSString class]]) {
                    TiStringRef propertyName = TiStringCreateWithCFString((CFStringRef)key);
                    TiObjectSetProperty(contextRef, js, propertyName, convertor.convert([dictionary objectForKey:key]), 0, 0);
                    TiStringRelease(propertyName);
                }
            }
        }
        
    } while (!convertor.isWorkListEmpty());

    return task.js;
}

TiValueRef valueInternalValue(TiValue * value)
{
    return value->m_value;
}

+ (TiValue *)valueWithTiValueRef:(TiValueRef)value inContext:(TiContext *)context
{
    return [context wrapperForJSObject:value];
}

- (TiValue *)init
{
    return nil;
}

- (TiValue *)initWithValue:(TiValueRef)value inContext:(TiContext *)context
{
    if (!value || !context)
        return nil;

    self = [super init];
    if (!self)
        return nil;

    _context = [context retain];
    m_value = value;
    TiValueProtect([_context TiGlobalContextRef], m_value);
    return self;
}

struct StructTagHandler {
    SEL typeToValueSEL;
    SEL valueToTypeSEL;
};
typedef HashMap<String, StructTagHandler> StructHandlers;

static StructHandlers* createStructHandlerMap()
{
    StructHandlers* structHandlers = new StructHandlers();

    size_t valueWithXinContextLength = strlen("valueWithX:inContext:");
    size_t toXLength = strlen("toX");

    // Step 1: find all valueWith<Foo>:inContext: class methods in TiValue.
    forEachMethodInClass(object_getClass([TiValue class]), ^(Method method){
        SEL selector = method_getName(method);
        const char* name = sel_getName(selector);
        size_t nameLength = strlen(name);
        // Check for valueWith<Foo>:context:
        if (nameLength < valueWithXinContextLength || memcmp(name, "valueWith", 9) || memcmp(name + nameLength - 11, ":inContext:", 11))
            return;
        // Check for [ id, SEL, <type>, <contextType> ]
        if (method_getNumberOfArguments(method) != 4)
            return;
        char idType[3];
        // Check 2nd argument type is "@"
        char* secondType = method_copyArgumentType(method, 3);
        if (strcmp(secondType, "@") != 0) {
            free(secondType);
            return;
        }
        free(secondType);
        // Check result type is also "@"
        method_getReturnType(method, idType, 3);
        if (strcmp(idType, "@") != 0)
            return;
        char* type = method_copyArgumentType(method, 2);
        structHandlers->add(StringImpl::create(type), (StructTagHandler){ selector, 0 });
        free(type);
    });

    // Step 2: find all to<Foo> instance methods in TiValue.
    forEachMethodInClass([TiValue class], ^(Method method){
        SEL selector = method_getName(method);
        const char* name = sel_getName(selector);
        size_t nameLength = strlen(name);
        // Check for to<Foo>
        if (nameLength < toXLength || memcmp(name, "to", 2))
            return;
        // Check for [ id, SEL ]
        if (method_getNumberOfArguments(method) != 2)
            return;
        // Try to find a matching valueWith<Foo>:context: method.
        char* type = method_copyReturnType(method);

        StructHandlers::iterator iter = structHandlers->find(type);
        free(type);
        if (iter == structHandlers->end())
            return;
        StructTagHandler& handler = iter->value;

        // check that strlen(<foo>) == strlen(<Foo>)
        const char* valueWithName = sel_getName(handler.typeToValueSEL);
        size_t valueWithLength = strlen(valueWithName);
        if (valueWithLength - valueWithXinContextLength != nameLength - toXLength)
            return;
        // Check that <Foo> == <Foo>
        if (memcmp(valueWithName + 9, name + 2, nameLength - toXLength - 1))
            return;
        handler.valueToTypeSEL = selector;
    });

    // Step 3: clean up - remove entries where we found prospective valueWith<Foo>:inContext: conversions, but no matching to<Foo> methods.
    typedef HashSet<String> RemoveSet;
    RemoveSet removeSet;
    for (StructHandlers::iterator iter = structHandlers->begin(); iter != structHandlers->end(); ++iter) {
        StructTagHandler& handler = iter->value;
        if (!handler.valueToTypeSEL)
            removeSet.add(iter->key);
    }

    for (RemoveSet::iterator iter = removeSet.begin(); iter != removeSet.end(); ++iter)
        structHandlers->remove(*iter);

    return structHandlers;
}

static StructTagHandler* handerForStructTag(const char* encodedType)
{
    static SpinLock handerForStructTagLock = SPINLOCK_INITIALIZER;
    SpinLockHolder lockHolder(&handerForStructTagLock);

    static StructHandlers* structHandlers = createStructHandlerMap();

    StructHandlers::iterator iter = structHandlers->find(encodedType);
    if (iter == structHandlers->end())
        return 0;
    return &iter->value;
}

+ (SEL)selectorForStructToValue:(const char *)structTag
{
    StructTagHandler* handler = handerForStructTag(structTag);
    return handler ? handler->typeToValueSEL : nil;
}

+ (SEL)selectorForValueToStruct:(const char *)structTag
{
    StructTagHandler* handler = handerForStructTag(structTag);
    return handler ? handler->valueToTypeSEL : nil;
}

- (void)dealloc
{
    TiValueUnprotect([_context TiGlobalContextRef], m_value);
    [_context release];
    _context = nil;
    [super dealloc];
}

- (NSString *)description
{
    if (id wrapped = tryUnwrapObjcObject([_context TiGlobalContextRef], m_value))
        return [wrapped description];
    return [self toString];
}

NSInvocation *typeToValueInvocationFor(const char* encodedType)
{
    SEL selector = [TiValue selectorForStructToValue:encodedType];
    if (!selector)
        return 0;

    const char* methodTypes = method_getTypeEncoding(class_getClassMethod([TiValue class], selector));
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[NSMethodSignature signatureWithObjCTypes:methodTypes]];
    [invocation setSelector:selector];
    return invocation;
}

NSInvocation *valueToTypeInvocationFor(const char* encodedType)
{
    SEL selector = [TiValue selectorForValueToStruct:encodedType];
    if (!selector)
        return 0;

    const char* methodTypes = method_getTypeEncoding(class_getInstanceMethod([TiValue class], selector));
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[NSMethodSignature signatureWithObjCTypes:methodTypes]];
    [invocation setSelector:selector];
    return invocation;
}

@end

#endif
