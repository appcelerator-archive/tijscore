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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CurrentThisInsideBlockGetterTest.h"

#if JSC_OBJC_API_ENABLED

#import <Foundation/Foundation.h>
#import <JavaScriptCore/TiCore.h>

static TiObjectRef CallAsConstructor(TiContextRef ctx, TiObjectRef constructor, size_t, const TiValueRef[], TiValueRef*)
{
    TiObjectRef newObjectRef = NULL;
    NSMutableDictionary *constructorPrivateProperties = (__bridge NSMutableDictionary *)(TiObjectGetPrivate(constructor));
    NSDictionary *constructorDescriptor = constructorPrivateProperties[@"constructorDescriptor"];
    newObjectRef = TiObjectMake(ctx, NULL, NULL);
    NSDictionary *objectProperties = constructorDescriptor[@"objectProperties"];
    
    if (objectProperties) {
        TiValue *newObject = [TiValue valueWithTiValueRef:newObjectRef inContext:[TiContext contextWithTiGlobalContextRef:TiContextGetGlobalContext(ctx)]];
        for (NSString *objectProperty in objectProperties) {
            [newObject defineProperty:objectProperty descriptor:objectProperties[objectProperty]];
        }
    }
    
    return newObjectRef;
}

static void ConstructorFinalize(TiObjectRef object)
{
    NSMutableDictionary *privateProperties = (__bridge NSMutableDictionary *)(TiObjectGetPrivate(object));
    CFBridgingRelease((__bridge CFTypeRef)(privateProperties));
    TiObjectSetPrivate(object, NULL);
}

static TiClassRef ConstructorClass(void)
{
    static TiClassRef constructorClass = NULL;
    
    if (constructorClass == NULL) {
        TiClassDefinition classDefinition = kTiClassDefinitionEmpty;
        classDefinition.className = "Constructor";
        classDefinition.callAsConstructor = CallAsConstructor;
        classDefinition.finalize = ConstructorFinalize;
        constructorClass = TiClassCreate(&classDefinition);
    }
    
    return constructorClass;
}

@interface TiValue (ConstructorCreation)

+ (TiValue *)valueWithConstructorDescriptor:(NSDictionary *)constructorDescriptor inContext:(TiContext *)context;

@end

@implementation TiValue (ConstructorCreation)

+ (TiValue *)valueWithConstructorDescriptor:(id)constructorDescriptor inContext:(TiContext *)context
{
    NSMutableDictionary *privateProperties = [@{ @"constructorDescriptor" : constructorDescriptor } mutableCopy];
    TiGlobalContextRef ctx = [context TiGlobalContextRef];
    TiObjectRef constructorRef = TiObjectMake(ctx, ConstructorClass(), (void *)CFBridgingRetain(privateProperties));
    TiValue *constructor = [TiValue valueWithTiValueRef:constructorRef inContext:context];
    return constructor;
}

@end

@interface TiContext (ConstructorCreation)

- (TiValue *)valueWithConstructorDescriptor:(NSDictionary *)constructorDescriptor;

@end

@implementation TiContext (ConstructorCreation)

- (TiValue *)valueWithConstructorDescriptor:(id)constructorDescriptor
{
    return [TiValue valueWithConstructorDescriptor:constructorDescriptor inContext:self];
}

@end

void currentThisInsideBlockGetterTest()
{
    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        
        TiValue *myConstructor = [context valueWithConstructorDescriptor:@{
            @"objectProperties" : @{
                @"currentThis" : @{ JSPropertyDescriptorGetKey : ^{ return TiContext.currentThis; } },
            },
        }];
        
        TiValue *myObj1 = [myConstructor constructWithArguments:nil];
        NSLog(@"myObj1.currentThis: %@", myObj1[@"currentThis"]);
        TiValue *myObj2 = [myConstructor constructWithArguments:@[ @"bar" ]];
        NSLog(@"myObj2.currentThis: %@", myObj2[@"currentThis"]);
    }
}

#endif // JSC_OBJC_API_ENABLED
