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
#import "TiCore.h"

#if JSC_OBJC_API_ENABLED

#import "APICast.h"
#import "APIShims.h"
#import "JSAPIWrapperObject.h"
#import "JSCallbackObject.h"
#import "TiContextInternal.h"
#import "JSWrapperMap.h"
#import "ObjCCallbackFunction.h"
#import "ObjcRuntimeExtras.h"
#import "Operations.h"
#import "WeakGCMap.h"
#import <wtf/TCSpinLock.h>
#import <wtf/Vector.h>
#import <wtf/HashSet.h>

#include <mach-o/dyld.h>

static const int32_t webkitFirstVersionWithInitConstructorSupport = 0x21A0400; // 538.4.0

@class JSObjCClassInfo;

@interface JSWrapperMap () 

- (JSObjCClassInfo*)classInfoForClass:(Class)cls;

@end

// Default conversion of selectors to property names.
// All semicolons are removed, lowercase letters following a semicolon are capitalized.
static NSString *selectorToPropertyName(const char* start)
{
    // Use 'index' to check for colons, if there are none, this is easy!
    const char* firstColon = index(start, ':');
    if (!firstColon)
        return [NSString stringWithUTF8String:start];

    // 'header' is the length of string up to the first colon.
    size_t header = firstColon - start;
    // The new string needs to be long enough to hold 'header', plus the remainder of the string, excluding
    // at least one ':', but including a '\0'. (This is conservative if there are more than one ':').
    char* buffer = static_cast<char*>(malloc(header + strlen(firstColon + 1) + 1));
    // Copy 'header' characters, set output to point to the end of this & input to point past the first ':'.
    memcpy(buffer, start, header);
    char* output = buffer + header;
    const char* input = start + header + 1;

    // On entry to the loop, we have already skipped over a ':' from the input.
    while (true) {
        char c;
        // Skip over any additional ':'s. We'll leave c holding the next character after the
        // last ':', and input pointing past c.
        while ((c = *(input++)) == ':');
        // Copy the character, converting to upper case if necessary.
        // If the character we copy is '\0', then we're done!
        if (!(*(output++) = toupper(c)))
            goto done;
        // Loop over characters other than ':'.
        while ((c = *(input++)) != ':') {
            // Copy the character.
            // If the character we copy is '\0', then we're done!
            if (!(*(output++) = c))
                goto done;
        }
        // If we get here, we've consumed a ':' - wash, rinse, repeat.
    }
done:
    NSString *result = [NSString stringWithUTF8String:buffer];
    free(buffer);
    return result;
}

static TiObjectRef makeWrapper(TiContextRef ctx, TiClassRef jsClass, id wrappedObject)
{
    TI::ExecState* exec = toJS(ctx);
    TI::APIEntryShim entryShim(exec);

    ASSERT(jsClass);
    TI::JSCallbackObject<TI::JSAPIWrapperObject>* object = TI::JSCallbackObject<TI::JSAPIWrapperObject>::create(exec, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->objcWrapperObjectStructure(), jsClass, 0);
    object->setWrappedObject(wrappedObject);
    if (TI::JSObject* prototype = jsClass->prototype(exec))
        object->setPrototype(exec->vm(), prototype);

    return toRef(object);
}

// Make an object that is in all ways a completely vanilla JavaScript object,
// other than that it has a native brand set that will be displayed by the default
// Object.prototype.toString conversion.
static TiValue *objectWithCustomBrand(TiContext *context, NSString *brand, Class cls = 0)
{
    TiClassDefinition definition;
    definition = kTiClassDefinitionEmpty;
    definition.className = [brand UTF8String];
    TiClassRef classRef = TiClassCreate(&definition);
    TiObjectRef result = makeWrapper([context TiGlobalContextRef], classRef, cls);
    TiClassRelease(classRef);
    return [TiValue valueWithTiValueRef:result inContext:context];
}

// Look for @optional properties in the prototype containing a selector to property
// name mapping, separated by a __JS_EXPORT_AS__ delimiter.
static NSMutableDictionary *createRenameMap(Protocol *protocol, BOOL isInstanceMethod)
{
    NSMutableDictionary *renameMap = [[NSMutableDictionary alloc] init];

    forEachMethodInProtocol(protocol, NO, isInstanceMethod, ^(SEL sel, const char*){
        NSString *rename = @(sel_getName(sel));
        NSRange range = [rename rangeOfString:@"__JS_EXPORT_AS__"];
        if (range.location == NSNotFound)
            return;
        NSString *selector = [rename substringToIndex:range.location];
        NSUInteger begin = range.location + range.length;
        NSUInteger length = [rename length] - begin - 1;
        NSString *name = [rename substringWithRange:(NSRange){ begin, length }];
        renameMap[selector] = name;
    });

    return renameMap;
}

inline void putNonEnumerable(TiValue *base, NSString *propertyName, TiValue *value)
{
    [base defineProperty:propertyName descriptor:@{
        JSPropertyDescriptorValueKey: value,
        JSPropertyDescriptorWritableKey: @YES,
        JSPropertyDescriptorEnumerableKey: @NO,
        JSPropertyDescriptorConfigurableKey: @YES
    }];
}

static bool isInitFamilyMethod(NSString *name)
{
    NSUInteger i = 0;

    // Skip over initial underscores.
    for (; i < [name length]; ++i) {
        if ([name characterAtIndex:i] != '_')
            break;
    }

    // Match 'init'.
    NSUInteger initIndex = 0;
    NSString* init = @"init";
    for (; i < [name length] && initIndex < [init length]; ++i, ++initIndex) {
        if ([name characterAtIndex:i] != [init characterAtIndex:initIndex])
            return false;
    }

    // We didn't match all of 'init'.
    if (initIndex < [init length])
        return false;

    // If we're at the end or the next character is a capital letter then this is an init-family selector.
    return i == [name length] || [[NSCharacterSet uppercaseLetterCharacterSet] characterIsMember:[name characterAtIndex:i]]; 
}

static bool shouldSkipMethodWithName(NSString *name)
{
    // For clients that don't support init-based constructors just copy 
    // over the init method as we would have before.
    if (!supportsInitMethodConstructors())
        return false;

    // Skip over init family methods because we handle those specially 
    // for the purposes of hooking up the constructor correctly.
    return isInitFamilyMethod(name);
}

// This method will iterate over the set of required methods in the protocol, and:
//  * Determine a property name (either via a renameMap or default conversion).
//  * If an accessorMap is provided, and contains this name, store the method in the map.
//  * Otherwise, if the object doesn't already contain a property with name, create it.
static void copyMethodsToObject(TiContext *context, Class objcClass, Protocol *protocol, BOOL isInstanceMethod, TiValue *object, NSMutableDictionary *accessorMethods = nil)
{
    NSMutableDictionary *renameMap = createRenameMap(protocol, isInstanceMethod);

    forEachMethodInProtocol(protocol, YES, isInstanceMethod, ^(SEL sel, const char* types){
        const char* nameCStr = sel_getName(sel);
        NSString *name = @(nameCStr);

        if (shouldSkipMethodWithName(name))
            return;

        if (accessorMethods && accessorMethods[name]) {
            TiObjectRef method = objCCallbackFunctionForMethod(context, objcClass, protocol, isInstanceMethod, sel, types);
            if (!method)
                return;
            accessorMethods[name] = [TiValue valueWithTiValueRef:method inContext:context];
        } else {
            name = renameMap[name];
            if (!name)
                name = selectorToPropertyName(nameCStr);
            if ([object hasProperty:name])
                return;
            TiObjectRef method = objCCallbackFunctionForMethod(context, objcClass, protocol, isInstanceMethod, sel, types);
            if (method)
                putNonEnumerable(object, name, [TiValue valueWithTiValueRef:method inContext:context]);
        }
    });

    [renameMap release];
}

static bool parsePropertyAttributes(objc_property_t property, char*& getterName, char*& setterName)
{
    bool readonly = false;
    unsigned attributeCount;
    objc_property_attribute_t* attributes = property_copyAttributeList(property, &attributeCount);
    if (attributeCount) {
        for (unsigned i = 0; i < attributeCount; ++i) {
            switch (*(attributes[i].name)) {
            case 'G':
                getterName = strdup(attributes[i].value);
                break;
            case 'S':
                setterName = strdup(attributes[i].value);
                break;
            case 'R':
                readonly = true;
                break;
            default:
                break;
            }
        }
        free(attributes);
    }
    return readonly;
}

static char* makeSetterName(const char* name)
{
    size_t nameLength = strlen(name);
    char* setterName = (char*)malloc(nameLength + 5); // "set" Name ":\0"
    setterName[0] = 's';
    setterName[1] = 'e';
    setterName[2] = 't';
    setterName[3] = toupper(*name);
    memcpy(setterName + 4, name + 1, nameLength - 1);
    setterName[nameLength + 3] = ':';
    setterName[nameLength + 4] = '\0';
    return setterName;
}

static void copyPrototypeProperties(TiContext *context, Class objcClass, Protocol *protocol, TiValue *prototypeValue)
{
    // First gather propreties into this list, then handle the methods (capturing the accessor methods).
    struct Property {
        const char* name;
        char* getterName;
        char* setterName;
    };
    __block Vector<Property> propertyList;

    // Map recording the methods used as getters/setters.
    NSMutableDictionary *accessorMethods = [NSMutableDictionary dictionary];

    // Useful value.
    TiValue *undefined = [TiValue valueWithUndefinedInContext:context];

    forEachPropertyInProtocol(protocol, ^(objc_property_t property){
        char* getterName = 0;
        char* setterName = 0;
        bool readonly = parsePropertyAttributes(property, getterName, setterName);
        const char* name = property_getName(property);

        // Add the names of the getter & setter methods to 
        if (!getterName)
            getterName = strdup(name);
        accessorMethods[@(getterName)] = undefined;
        if (!readonly) {
            if (!setterName)
                setterName = makeSetterName(name);
            accessorMethods[@(setterName)] = undefined;
        }

        // Add the properties to a list.
        propertyList.append((Property){ name, getterName, setterName });
    });

    // Copy methods to the prototype, capturing accessors in the accessorMethods map.
    copyMethodsToObject(context, objcClass, protocol, YES, prototypeValue, accessorMethods);

    // Iterate the propertyList & generate accessor properties.
    for (size_t i = 0; i < propertyList.size(); ++i) {
        Property& property = propertyList[i];

        TiValue *getter = accessorMethods[@(property.getterName)];
        free(property.getterName);
        ASSERT(![getter isUndefined]);

        TiValue *setter = undefined;
        if (property.setterName) {
            setter = accessorMethods[@(property.setterName)];
            free(property.setterName);
            ASSERT(![setter isUndefined]);
        }
        
        [prototypeValue defineProperty:@(property.name) descriptor:@{
            JSPropertyDescriptorGetKey: getter,
            JSPropertyDescriptorSetKey: setter,
            JSPropertyDescriptorEnumerableKey: @NO,
            JSPropertyDescriptorConfigurableKey: @YES
        }];
    }
}

@interface JSObjCClassInfo : NSObject {
    TiContext *m_context;
    Class m_class;
    bool m_block;
    TiClassRef m_classRef;
    TI::Weak<TI::JSObject> m_prototype;
    TI::Weak<TI::JSObject> m_constructor;
}

- (id)initWithContext:(TiContext *)context forClass:(Class)cls superClassInfo:(JSObjCClassInfo*)superClassInfo;
- (TiValue *)wrapperForObject:(id)object;
- (TiValue *)constructor;

@end

@implementation JSObjCClassInfo

- (id)initWithContext:(TiContext *)context forClass:(Class)cls superClassInfo:(JSObjCClassInfo*)superClassInfo
{
    self = [super init];
    if (!self)
        return nil;

    const char* className = class_getName(cls);
    m_context = context;
    m_class = cls;
    m_block = [cls isSubclassOfClass:getNSBlockClass()];
    TiClassDefinition definition;
    definition = kTiClassDefinitionEmpty;
    definition.className = className;
    m_classRef = TiClassCreate(&definition);

    [self allocateConstructorAndPrototypeWithSuperClassInfo:superClassInfo];

    return self;
}

- (void)dealloc
{
    TiClassRelease(m_classRef);
    [super dealloc];
}

static TiValue *allocateConstructorForCustomClass(TiContext *context, const char* className, Class cls)
{
    if (!supportsInitMethodConstructors())
        return objectWithCustomBrand(context, [NSString stringWithFormat:@"%sConstructor", className], cls);

    // For each protocol that the class implements, gather all of the init family methods into a hash table.
    __block HashMap<String, Protocol *> initTable;
    Protocol *exportProtocol = getTiExportProtocol();
    for (Class currentClass = cls; currentClass; currentClass = class_getSuperclass(currentClass)) {
        forEachProtocolImplementingProtocol(currentClass, exportProtocol, ^(Protocol *protocol) {
            forEachMethodInProtocol(protocol, YES, YES, ^(SEL selector, const char*) {
                const char* name = sel_getName(selector);
                if (!isInitFamilyMethod(@(name)))
                    return;
                initTable.set(name, protocol);
            });
        });
    }

    for (Class currentClass = cls; currentClass; currentClass = class_getSuperclass(currentClass)) {
        __block unsigned numberOfInitsFound = 0;
        __block SEL initMethod = 0;
        __block Protocol *initProtocol = 0;
        __block const char* types = 0;
        forEachMethodInClass(currentClass, ^(Method method) {
            SEL selector = method_getName(method);
            const char* name = sel_getName(selector);
            auto iter = initTable.find(name);

            if (iter == initTable.end())
                return;

            numberOfInitsFound++;
            initMethod = selector;
            initProtocol = iter->value;
            types = method_getTypeEncoding(method);
        });

        if (!numberOfInitsFound)
            continue;

        if (numberOfInitsFound > 1) {
            NSLog(@"ERROR: Class %@ exported more than one init family method via TiExport. Class %@ will not have a callable JavaScript constructor function.", cls, cls);
            break;
        }

        TiObjectRef method = objCCallbackFunctionForInit(context, cls, initProtocol, initMethod, types);
        return [TiValue valueWithTiValueRef:method inContext:context];
    }
    return objectWithCustomBrand(context, [NSString stringWithFormat:@"%sConstructor", className], cls);
}

- (void)allocateConstructorAndPrototypeWithSuperClassInfo:(JSObjCClassInfo*)superClassInfo
{
    ASSERT(!m_constructor || !m_prototype);
    ASSERT((m_class == [NSObject class]) == !superClassInfo);
    if (!superClassInfo) {
        TiContextRef cContext = [m_context TiGlobalContextRef];
        TiValue *constructor = m_context[@"Object"];
        if (!m_constructor)
            m_constructor = toJS(TiValueToObject(cContext, valueInternalValue(constructor), 0));

        if (!m_prototype) {
            TiValue *prototype = constructor[@"prototype"];
            m_prototype = toJS(TiValueToObject(cContext, valueInternalValue(prototype), 0));
        }
    } else {
        const char* className = class_getName(m_class);

        // Create or grab the prototype/constructor pair.
        TiValue *prototype;
        TiValue *constructor;
        if (m_prototype)
            prototype = [TiValue valueWithTiValueRef:toRef(m_prototype.get()) inContext:m_context];
        else
            prototype = objectWithCustomBrand(m_context, [NSString stringWithFormat:@"%sPrototype", className]);

        if (m_constructor)
            constructor = [TiValue valueWithTiValueRef:toRef(m_constructor.get()) inContext:m_context];
        else
            constructor = allocateConstructorForCustomClass(m_context, className, m_class);

        TiContextRef cContext = [m_context TiGlobalContextRef];
        m_prototype = toJS(TiValueToObject(cContext, valueInternalValue(prototype), 0));
        m_constructor = toJS(TiValueToObject(cContext, valueInternalValue(constructor), 0));

        putNonEnumerable(prototype, @"constructor", constructor);
        putNonEnumerable(constructor, @"prototype", prototype);

        Protocol *exportProtocol = getTiExportProtocol();
        forEachProtocolImplementingProtocol(m_class, exportProtocol, ^(Protocol *protocol){
            copyPrototypeProperties(m_context, m_class, protocol, prototype);
            copyMethodsToObject(m_context, m_class, protocol, NO, constructor);
        });

        // Set [Prototype].
        TiObjectSetPrototype([m_context TiGlobalContextRef], toRef(m_prototype.get()), toRef(superClassInfo->m_prototype.get()));
    }
}

- (void)reallocateConstructorAndOrPrototype
{
    [self allocateConstructorAndPrototypeWithSuperClassInfo:[m_context.wrapperMap classInfoForClass:class_getSuperclass(m_class)]];
}

- (TiValue *)wrapperForObject:(id)object
{
    ASSERT([object isKindOfClass:m_class]);
    ASSERT(m_block == [object isKindOfClass:getNSBlockClass()]);
    if (m_block) {
        if (TiObjectRef method = objCCallbackFunctionForBlock(m_context, object)) {
            TiValue *constructor = [TiValue valueWithTiValueRef:method inContext:m_context];
            TiValue *prototype = [TiValue valueWithNewObjectInContext:m_context];
            putNonEnumerable(constructor, @"prototype", prototype);
            putNonEnumerable(prototype, @"constructor", constructor);
            return constructor;
        }
    }

    if (!m_prototype)
        [self reallocateConstructorAndOrPrototype];
    ASSERT(!!m_prototype);

    TiObjectRef wrapper = makeWrapper([m_context TiGlobalContextRef], m_classRef, object);
    TiObjectSetPrototype([m_context TiGlobalContextRef], wrapper, toRef(m_prototype.get()));
    return [TiValue valueWithTiValueRef:wrapper inContext:m_context];
}

- (TiValue *)constructor
{
    if (!m_constructor)
        [self reallocateConstructorAndOrPrototype];
    ASSERT(!!m_constructor);
    return [TiValue valueWithTiValueRef:toRef(m_constructor.get()) inContext:m_context];
}

@end

@implementation JSWrapperMap {
    TiContext *m_context;
    NSMutableDictionary *m_classMap;
    TI::WeakGCMap<id, TI::JSObject> m_cachedJSWrappers;
    NSMapTable *m_cachedObjCWrappers;
}

- (id)initWithContext:(TiContext *)context
{
    self = [super init];
    if (!self)
        return nil;

    NSPointerFunctionsOptions keyOptions = NSPointerFunctionsOpaqueMemory | NSPointerFunctionsOpaquePersonality;
    NSPointerFunctionsOptions valueOptions = NSPointerFunctionsWeakMemory | NSPointerFunctionsObjectPersonality;
    m_cachedObjCWrappers = [[NSMapTable alloc] initWithKeyOptions:keyOptions valueOptions:valueOptions capacity:0];
    
    m_context = context;
    m_classMap = [[NSMutableDictionary alloc] init];
    return self;
}

- (void)dealloc
{
    [m_cachedObjCWrappers release];
    [m_classMap release];
    [super dealloc];
}

- (JSObjCClassInfo*)classInfoForClass:(Class)cls
{
    if (!cls)
        return nil;

    // Check if we've already created a JSObjCClassInfo for this Class.
    if (JSObjCClassInfo* classInfo = (JSObjCClassInfo*)m_classMap[cls])
        return classInfo;

    // Skip internal classes beginning with '_' - just copy link to the parent class's info.
    if ('_' == *class_getName(cls))
        return m_classMap[cls] = [self classInfoForClass:class_getSuperclass(cls)];

    return m_classMap[cls] = [[[JSObjCClassInfo alloc] initWithContext:m_context forClass:cls superClassInfo:[self classInfoForClass:class_getSuperclass(cls)]] autorelease];
}

- (TiValue *)jsWrapperForObject:(id)object
{
    TI::JSObject* jsWrapper = m_cachedJSWrappers.get(object);
    if (jsWrapper)
        return [TiValue valueWithTiValueRef:toRef(jsWrapper) inContext:m_context];

    TiValue *wrapper;
    if (class_isMetaClass(object_getClass(object)))
        wrapper = [[self classInfoForClass:(Class)object] constructor];
    else {
        JSObjCClassInfo* classInfo = [self classInfoForClass:[object class]];
        wrapper = [classInfo wrapperForObject:object];
    }

    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=105891
    // This general approach to wrapper caching is pretty effective, but there are a couple of problems:
    // (1) For immortal objects TiValues will effectively leak and this results in error output being logged - we should avoid adding associated objects to immortal objects.
    // (2) A long lived object may rack up many TiValues. When the contexts are released these will unprotect the associated JavaScript objects,
    //     but still, would probably nicer if we made it so that only one associated object was required, broadcasting object dealloc.
    TI::ExecState* exec = toJS([m_context TiGlobalContextRef]);
    jsWrapper = toJS(exec, valueInternalValue(wrapper)).toObject(exec);
    m_cachedJSWrappers.set(object, jsWrapper);
    return wrapper;
}

- (TiValue *)objcWrapperForTiValueRef:(TiValueRef)value
{
    TiValue *wrapper = static_cast<TiValue *>(NSMapGet(m_cachedObjCWrappers, value));
    if (!wrapper) {
        wrapper = [[[TiValue alloc] initWithValue:value inContext:m_context] autorelease];
        NSMapInsert(m_cachedObjCWrappers, value, wrapper);
    }
    return wrapper;
}

@end

id tryUnwrapObjcObject(TiGlobalContextRef context, TiValueRef value)
{
    if (!TiValueIsObject(context, value))
        return nil;
    TiValueRef exception = 0;
    TiObjectRef object = TiValueToObject(context, value, &exception);
    ASSERT(!exception);
    if (toJS(object)->inherits(TI::JSCallbackObject<TI::JSAPIWrapperObject>::info()))
        return (id)TI::jsCast<TI::JSAPIWrapperObject*>(toJS(object))->wrappedObject();
    if (id target = tryUnwrapConstructor(object))
        return target;
    return nil;
}

// This class ensures that the TiExport protocol is registered with the runtime.
NS_ROOT_CLASS @interface TiExport <TiExport>
@end
@implementation TiExport
@end

bool supportsInitMethodConstructors()
{
    static int32_t versionOfLinkTimeLibrary = 0;
    if (!versionOfLinkTimeLibrary)
        versionOfLinkTimeLibrary = NSVersionOfLinkTimeLibrary("JavaScriptCore");
    return versionOfLinkTimeLibrary >= webkitFirstVersionWithInitConstructorSupport;
}

Protocol *getTiExportProtocol()
{
    static Protocol *protocol = objc_getProtocol("TiExport");
    return protocol;
}

Class getNSBlockClass()
{
    static Class cls = objc_getClass("NSBlock");
    return cls;
}

#endif
