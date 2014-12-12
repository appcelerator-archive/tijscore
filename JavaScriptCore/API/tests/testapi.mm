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

#import <JavaScriptCore/TiCore.h>

#import "CurrentThisInsideBlockGetterTest.h"

extern "C" void JSSynchronousGarbageCollectForDebugging(TiContextRef);

extern "C" bool _Block_has_signature(id);
extern "C" const char * _Block_signature(id);

extern int failed;
extern "C" void testObjectiveCAPI(void);

#if JSC_OBJC_API_ENABLED

@protocol ParentObject <TiExport>
@end

@interface ParentObject : NSObject<ParentObject>
+ (NSString *)parentTest;
@end

@implementation ParentObject
+ (NSString *)parentTest
{
    return [self description];
}
@end

@protocol TestObject <TiExport>
- (id)init;
@property int variable;
@property (readonly) int six;
@property CGPoint point;
+ (NSString *)classTest;
+ (NSString *)parentTest;
- (NSString *)getString;
TiExportAs(testArgumentTypes,
- (NSString *)testArgumentTypesWithInt:(int)i double:(double)d boolean:(BOOL)b string:(NSString *)s number:(NSNumber *)n array:(NSArray *)a dictionary:(NSDictionary *)o
);
- (void)callback:(TiValue *)function;
- (void)bogusCallback:(void(^)(int))function;
@end

@interface TestObject : ParentObject <TestObject>
@property int six;
+ (id)testObject;
@end

@implementation TestObject
@synthesize variable;
@synthesize six;
@synthesize point;
+ (id)testObject
{
    return [[TestObject alloc] init];
}
+ (NSString *)classTest
{
    return @"classTest - okay";
}
- (NSString *)getString
{
    return @"42";
}
- (NSString *)testArgumentTypesWithInt:(int)i double:(double)d boolean:(BOOL)b string:(NSString *)s number:(NSNumber *)n array:(NSArray *)a dictionary:(NSDictionary *)o
{
    return [NSString stringWithFormat:@"%d,%g,%d,%@,%d,%@,%@", i, d, b==YES?true:false,s,[n intValue],a[1],o[@"x"]];
}
- (void)callback:(TiValue *)function
{
    [function callWithArguments:[NSArray arrayWithObject:[NSNumber numberWithInt:42]]];
}
- (void)bogusCallback:(void(^)(int))function
{
    function(42);
}
@end

bool testXYZTested = false;

@protocol TextXYZ <TiExport>
- (id)initWithString:(NSString*)string;
@property int x;
@property (readonly) int y;
@property (assign) TiValue *onclick;
@property (assign) TiValue *weakOnclick;
- (void)test:(NSString *)message;
@end

@interface TextXYZ : NSObject <TextXYZ>
@property int x;
@property int y;
@property int z;
- (void)click;
@end

@implementation TextXYZ {
    TiManagedValue *m_weakOnclickHandler;
    TiManagedValue *m_onclickHandler;
}
@synthesize x;
@synthesize y;
@synthesize z;
- (id)initWithString:(NSString*)string
{
    self = [super init];
    if (!self)
        return nil;

    NSLog(@"%@", string);

    return self;
}
- (void)test:(NSString *)message
{
    testXYZTested = [message isEqual:@"test"] && x == 13 & y == 4 && z == 5;
}
- (void)setWeakOnclick:(TiValue *)value
{
    m_weakOnclickHandler = [TiManagedValue managedValueWithValue:value];
}

- (void)setOnclick:(TiValue *)value
{
    m_onclickHandler = [TiManagedValue managedValueWithValue:value];
    [value.context.virtualMachine addManagedReference:m_onclickHandler withOwner:self];
}
- (TiValue *)weakOnclick
{
    return [m_weakOnclickHandler value];
}
- (TiValue *)onclick
{
    return [m_onclickHandler value];
}
- (void)click
{
    if (!m_onclickHandler)
        return;

    TiValue *function = [m_onclickHandler value];
    [function callWithArguments:[NSArray array]];
}
- (void)dealloc
{
    [[m_onclickHandler value].context.virtualMachine removeManagedReference:m_onclickHandler withOwner:self];
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}
@end

@class TinyDOMNode;

@protocol TinyDOMNode <TiExport>
- (void)appendChild:(TinyDOMNode *)child;
- (NSUInteger)numberOfChildren;
- (TinyDOMNode *)childAtIndex:(NSUInteger)index;
- (void)removeChildAtIndex:(NSUInteger)index;
@end

@interface TinyDOMNode : NSObject<TinyDOMNode>
@end

@implementation TinyDOMNode {
    NSMutableArray *m_children;
    TiVirtualMachine *m_sharedVirtualMachine;
}

- (id)initWithVirtualMachine:(TiVirtualMachine *)virtualMachine
{
    self = [super init];
    if (!self)
        return nil;

    m_children = [[NSMutableArray alloc] initWithCapacity:0];
    m_sharedVirtualMachine = virtualMachine;
#if !__has_feature(objc_arc)
    [m_sharedVirtualMachine retain];
#endif

    return self;
}

- (void)dealloc
{
    for (TinyDOMNode *child in m_children)
        [m_sharedVirtualMachine removeManagedReference:child withOwner:self];

#if !__has_feature(objc_arc)
    [m_children release];
    [m_sharedVirtualMachine release];
    [super dealloc];
#endif
}

- (void)appendChild:(TinyDOMNode *)child
{
    [m_sharedVirtualMachine addManagedReference:child withOwner:self];
    [m_children addObject:child];
}

- (NSUInteger)numberOfChildren
{
    return [m_children count];
}

- (TinyDOMNode *)childAtIndex:(NSUInteger)index
{
    if (index >= [m_children count])
        return nil;
    return [m_children objectAtIndex:index];
}

- (void)removeChildAtIndex:(NSUInteger)index
{
    if (index >= [m_children count])
        return;
    [m_sharedVirtualMachine removeManagedReference:[m_children objectAtIndex:index] withOwner:self];
    [m_children removeObjectAtIndex:index];
}

@end

@interface JSCollection : NSObject
- (void)setValue:(TiValue *)value forKey:(NSString *)key;
- (TiValue *)valueForKey:(NSString *)key;
@end

@implementation JSCollection {
    NSMutableDictionary *_dict;
}
- (id)init
{
    self = [super init];
    if (!self)
        return nil;

    _dict = [[NSMutableDictionary alloc] init];

    return self;
}

- (void)setValue:(TiValue *)value forKey:(NSString *)key
{
    TiManagedValue *oldManagedValue = [_dict objectForKey:key];
    if (oldManagedValue) {
        TiValue* oldValue = [oldManagedValue value];
        if (oldValue)
            [oldValue.context.virtualMachine removeManagedReference:oldManagedValue withOwner:self];
    }
    TiManagedValue *managedValue = [TiManagedValue managedValueWithValue:value];
    [value.context.virtualMachine addManagedReference:managedValue withOwner:self];
    [_dict setObject:managedValue forKey:key];
}

- (TiValue *)valueForKey:(NSString *)key
{
    TiManagedValue *managedValue = [_dict objectForKey:key];
    if (!managedValue)
        return nil;
    return [managedValue value];
}
@end

@protocol InitA <TiExport>
- (id)initWithA:(int)a;
- (int)initialize;
@end

@protocol InitB <TiExport>
- (id)initWithA:(int)a b:(int)b;
@end

@protocol InitC <TiExport>
- (id)_init;
@end

@interface ClassA : NSObject<InitA>
@end

@interface ClassB : ClassA<InitB>
@end

@interface ClassC : ClassB<InitA, InitB>
@end

@interface ClassCPrime : ClassB<InitA, InitC>
@end

@interface ClassD : NSObject<InitA>
- (id)initWithA:(int)a;
@end

@interface ClassE : ClassD
- (id)initWithA:(int)a;
@end

@implementation ClassA {
    int _a;
}
- (id)initWithA:(int)a
{
    self = [super init];
    if (!self)
        return nil;

    _a = a;

    return self;
}
- (int)initialize
{
    return 42;
}
@end

@implementation ClassB {
    int _b;
}
- (id)initWithA:(int)a b:(int)b
{
    self = [super initWithA:a];
    if (!self)
        return nil;

    _b = b;

    return self;
}
@end

@implementation ClassC {
    int _c;
}
- (id)initWithA:(int)a
{
    return [self initWithA:a b:0];
}
- (id)initWithA:(int)a b:(int)b
{
    self = [super initWithA:a b:b];
    if (!self)
        return nil;

    _c = a + b;

    return self;
}
@end

@implementation ClassCPrime
- (id)initWithA:(int)a
{
    self = [super initWithA:a b:0];
    if (!self)
        return nil;
    return self;
}
- (id)_init
{
    return [self initWithA:42];
}
@end

@implementation ClassD

- (id)initWithA:(int)a
{
    self = nil;
    return [[ClassE alloc] initWithA:a];
}
- (int)initialize
{
    return 0;
}
@end

@implementation ClassE {
    int _a;
}

- (id)initWithA:(int)a
{
    self = [super init];
    if (!self)
        return nil;

    _a = a;

    return self;
}
@end

static bool evilAllocationObjectWasDealloced = false;

@interface EvilAllocationObject : NSObject
- (TiValue *)doEvilThingsWithContext:(TiContext *)context;
@end

@implementation EvilAllocationObject {
    TiContext *m_context;
}
- (id)initWithContext:(TiContext *)context
{
    self = [super init];
    if (!self)
        return nil;

    m_context = context;

    return self;
}
- (void)dealloc
{
    [self doEvilThingsWithContext:m_context];
    evilAllocationObjectWasDealloced = true;
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

- (TiValue *)doEvilThingsWithContext:(TiContext *)context
{
    return [context evaluateScript:@" \
        (function() { \
            var a = []; \
            var sum = 0; \
            for (var i = 0; i < 10000; ++i) { \
                sum += i; \
                a[i] = sum; \
            } \
            return sum; \
        })()"];
}
@end

static void checkResult(NSString *description, bool passed)
{
    NSLog(@"TEST: \"%@\": %@", description, passed ? @"PASSED" : @"FAILED");
    if (!passed)
        failed = 1;
}

static bool blockSignatureContainsClass()
{
    static bool containsClass = ^{
        id block = ^(NSString *string){ return string; };
        return _Block_has_signature(block) && strstr(_Block_signature(block), "NSString");
    }();
    return containsClass;
}

void testObjectiveCAPI()
{
    NSLog(@"Testing Objective-C API");

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TiValue *result = [context evaluateScript:@"2 + 2"];
        checkResult(@"2 + 2", [result isNumber] && [result toInt32] == 4);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        NSString *result = [NSString stringWithFormat:@"Two plus two is %@", [context evaluateScript:@"2 + 2"]];
        checkResult(@"stringWithFormat", [result isEqual:@"Two plus two is 4"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"message"] = @"Hello";
        TiValue *result = [context evaluateScript:@"message + ', World!'"];
        checkResult(@"Hello, World!", [result isString] && [result isEqualToObject:@"Hello, World!"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TiValue *result = [context evaluateScript:@"({ x:42 })"];
        checkResult(@"({ x:42 })", [result isObject] && [result[@"x"] isEqualToObject:@42]);
        id obj = [result toObject];
        checkResult(@"Check dictionary literal", [obj isKindOfClass:[NSDictionary class]]);
        id num = (NSDictionary *)obj[@"x"];
        checkResult(@"Check numeric literal", [num isKindOfClass:[NSNumber class]]);
    }

    @autoreleasepool {
        JSCollection* myPrivateProperties = [[JSCollection alloc] init];

        @autoreleasepool {
            TiContext* context = [[TiContext alloc] init];
            TestObject* rootObject = [TestObject testObject];
            context[@"root"] = rootObject;
            [context.virtualMachine addManagedReference:myPrivateProperties withOwner:rootObject];
            [myPrivateProperties setValue:[TiValue valueWithBool:true inContext:context] forKey:@"is_ham"];
            [myPrivateProperties setValue:[TiValue valueWithObject:@"hello!" inContext:context] forKey:@"message"];
            [myPrivateProperties setValue:[TiValue valueWithInt32:42 inContext:context] forKey:@"my_number"];
            [myPrivateProperties setValue:[TiValue valueWithNullInContext:context] forKey:@"definitely_null"];
            [myPrivateProperties setValue:[TiValue valueWithUndefinedInContext:context] forKey:@"not_sure_if_undefined"];

            JSSynchronousGarbageCollectForDebugging([context TiGlobalContextRef]);

            TiValue *isHam = [myPrivateProperties valueForKey:@"is_ham"];
            TiValue *message = [myPrivateProperties valueForKey:@"message"];
            TiValue *myNumber = [myPrivateProperties valueForKey:@"my_number"];
            TiValue *definitelyNull = [myPrivateProperties valueForKey:@"definitely_null"];
            TiValue *notSureIfUndefined = [myPrivateProperties valueForKey:@"not_sure_if_undefined"];
            checkResult(@"is_ham is true", [isHam isBoolean] && [isHam toBool]);
            checkResult(@"message is hello!", [message isString] && [@"hello!" isEqualToString:[message toString]]);
            checkResult(@"my_number is 42", [myNumber isNumber] && [myNumber toInt32] == 42);
            checkResult(@"definitely_null is null", [definitelyNull isNull]);
            checkResult(@"not_sure_if_undefined is undefined", [notSureIfUndefined isUndefined]);
        }

        checkResult(@"is_ham is nil", ![myPrivateProperties valueForKey:@"is_ham"]);
        checkResult(@"message is nil", ![myPrivateProperties valueForKey:@"message"]);
        checkResult(@"my_number is 42", ![myPrivateProperties valueForKey:@"my_number"]);
        checkResult(@"definitely_null is null", ![myPrivateProperties valueForKey:@"definitely_null"]);
        checkResult(@"not_sure_if_undefined is undefined", ![myPrivateProperties valueForKey:@"not_sure_if_undefined"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        __block int result;
        context[@"blockCallback"] = ^(int value){
            result = value;
        };
        [context evaluateScript:@"blockCallback(42)"];
        checkResult(@"blockCallback", result == 42);
    }

    if (blockSignatureContainsClass()) {
        @autoreleasepool {
            TiContext *context = [[TiContext alloc] init];
            __block bool result = false;
            context[@"blockCallback"] = ^(NSString *value){
                result = [@"42" isEqualToString:value] == YES;
            };
            [context evaluateScript:@"blockCallback(42)"];
            checkResult(@"blockCallback(NSString *)", result);
        }
    } else
        NSLog(@"Skipping 'blockCallback(NSString *)' test case");

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        checkResult(@"!context.exception", !context.exception);
        [context evaluateScript:@"!@#$%^&*() THIS IS NOT VALID JAVASCRIPT SYNTAX !@#$%^&*()"];
        checkResult(@"context.exception", context.exception);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        __block bool caught = false;
        context.exceptionHandler = ^(TiContext *context, TiValue *exception) {
            (void)context;
            (void)exception;
            caught = true;
        };
        [context evaluateScript:@"!@#$%^&*() THIS IS NOT VALID JAVASCRIPT SYNTAX !@#$%^&*()"];
        checkResult(@"TiContext.exceptionHandler", caught);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"callback"] = ^{
            TiContext *context = [TiContext currentContext];
            context.exception = [TiValue valueWithNewErrorFromMessage:@"Something went wrong." inContext:context];
        };
        TiValue *result = [context evaluateScript:@"var result; try { callback(); } catch (e) { result = 'Caught exception'; }"];
        checkResult(@"Explicit throw in callback - was caught by JavaScript", [result isEqualToObject:@"Caught exception"]);
        checkResult(@"Explicit throw in callback - not thrown to Objective-C", !context.exception);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"callback"] = ^{
            TiContext *context = [TiContext currentContext];
            [context evaluateScript:@"!@#$%^&*() THIS IS NOT VALID JAVASCRIPT SYNTAX !@#$%^&*()"];
        };
        TiValue *result = [context evaluateScript:@"var result; try { callback(); } catch (e) { result = 'Caught exception'; }"];
        checkResult(@"Implicit throw in callback - was caught by JavaScript", [result isEqualToObject:@"Caught exception"]);
        checkResult(@"Implicit throw in callback - not thrown to Objective-C", !context.exception);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        [context evaluateScript:
            @"function sum(array) { \
                var result = 0; \
                for (var i in array) \
                    result += array[i]; \
                return result; \
            }"];
        TiValue *array = [TiValue valueWithObject:@[@13, @2, @7] inContext:context];
        TiValue *sumFunction = context[@"sum"];
        TiValue *result = [sumFunction callWithArguments:@[ array ]];
        checkResult(@"sum([13, 2, 7])", [result toInt32] == 22);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TiValue *mulAddFunction = [context evaluateScript:
            @"(function(array, object) { \
                var result = []; \
                for (var i in array) \
                    result.push(array[i] * object.x + object.y); \
                return result; \
            })"];
        TiValue *result = [mulAddFunction callWithArguments:@[ @[ @2, @4, @8 ], @{ @"x":@0.5, @"y":@42 } ]];
        checkResult(@"mulAddFunction", [result isObject] && [[result toString] isEqual:@"43,44,46"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];        
        TiValue *array = [TiValue valueWithNewArrayInContext:context];
        checkResult(@"arrayLengthEmpty", [[array[@"length"] toNumber] unsignedIntegerValue] == 0);
        TiValue *value1 = [TiValue valueWithInt32:42 inContext:context];
        TiValue *value2 = [TiValue valueWithInt32:24 inContext:context];
        NSUInteger lowIndex = 5;
        NSUInteger maxLength = UINT_MAX;

        [array setValue:value1 atIndex:lowIndex];
        checkResult(@"array.length after put to low index", [[array[@"length"] toNumber] unsignedIntegerValue] == (lowIndex + 1));

        [array setValue:value1 atIndex:(maxLength - 1)];
        checkResult(@"array.length after put to maxLength - 1", [[array[@"length"] toNumber] unsignedIntegerValue] == maxLength);

        [array setValue:value2 atIndex:maxLength];
        checkResult(@"array.length after put to maxLength", [[array[@"length"] toNumber] unsignedIntegerValue] == maxLength);

        [array setValue:value2 atIndex:(maxLength + 1)];
        checkResult(@"array.length after put to maxLength + 1", [[array[@"length"] toNumber] unsignedIntegerValue] == maxLength);

        if (sizeof(NSUInteger) == 8)
            checkResult(@"valueAtIndex:0 is undefined", [[array valueAtIndex:0] isUndefined]);
        else
            checkResult(@"valueAtIndex:0", [[array valueAtIndex:0] toInt32] == 24);
        checkResult(@"valueAtIndex:lowIndex", [[array valueAtIndex:lowIndex] toInt32] == 42);
        checkResult(@"valueAtIndex:maxLength - 1", [[array valueAtIndex:(maxLength - 1)] toInt32] == 42);
        checkResult(@"valueAtIndex:maxLength", [[array valueAtIndex:maxLength] toInt32] == 24);
        checkResult(@"valueAtIndex:maxLength + 1", [[array valueAtIndex:(maxLength + 1)] toInt32] == 24);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TiValue *object = [TiValue valueWithNewObjectInContext:context];

        object[@"point"] = @{ @"x":@1, @"y":@2 };
        object[@"point"][@"x"] = @3;
        CGPoint point = [object[@"point"] toPoint];
        checkResult(@"toPoint", point.x == 3 && point.y == 2);

        object[@{ @"toString":^{ return @"foo"; } }] = @"bar";
        checkResult(@"toString in object literal used as subscript", [[object[@"foo"] toString] isEqual:@"bar"]);

        object[[@"foobar" substringToIndex:3]] = @"bar";
        checkResult(@"substring used as subscript", [[object[@"foo"] toString] isEqual:@"bar"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TextXYZ *testXYZ = [[TextXYZ alloc] init];
        context[@"testXYZ"] = testXYZ;
        testXYZ.x = 3;
        testXYZ.y = 4;
        testXYZ.z = 5;
        [context evaluateScript:@"testXYZ.x = 13; testXYZ.y = 14;"];
        [context evaluateScript:@"testXYZ.test('test')"];
        checkResult(@"TextXYZ - testXYZTested", testXYZTested);
        TiValue *result = [context evaluateScript:@"testXYZ.x + ',' + testXYZ.y + ',' + testXYZ.z"];
        checkResult(@"TextXYZ - result", [result isEqualToObject:@"13,4,undefined"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        [context[@"Object"][@"prototype"] defineProperty:@"getterProperty" descriptor:@{
            JSPropertyDescriptorGetKey:^{
                return [TiContext currentThis][@"x"];
            }
        }];
        TiValue *object = [TiValue valueWithObject:@{ @"x":@101 } inContext:context];
        int result = [object [@"getterProperty"] toInt32];
        checkResult(@"getterProperty", result == 101);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"concatenate"] = ^{
            NSArray *arguments = [TiContext currentArguments];
            if (![arguments count])
                return @"";
            NSString *message = [arguments[0] description];
            for (NSUInteger index = 1; index < [arguments count]; ++index)
                message = [NSString stringWithFormat:@"%@ %@", message, arguments[index]];
            return message;
        };
        TiValue *result = [context evaluateScript:@"concatenate('Hello,', 'World!')"];
        checkResult(@"concatenate", [result isEqualToObject:@"Hello, World!"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"foo"] = @YES;
        checkResult(@"@YES is boolean", [context[@"foo"] isBoolean]);
        TiValue *result = [context evaluateScript:@"typeof foo"];
        checkResult(@"@YES is boolean", [result isEqualToObject:@"boolean"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject* testObject = [TestObject testObject];
        context[@"testObject"] = testObject;
        TiValue *result = [context evaluateScript:@"String(testObject)"];
        checkResult(@"String(testObject)", [result isEqualToObject:@"[object TestObject]"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject* testObject = [TestObject testObject];
        context[@"testObject"] = testObject;
        TiValue *result = [context evaluateScript:@"String(testObject.__proto__)"];
        checkResult(@"String(testObject.__proto__)", [result isEqualToObject:@"[object TestObjectPrototype]"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"TestObject"] = [TestObject class];
        TiValue *result = [context evaluateScript:@"String(TestObject)"];
        checkResult(@"String(TestObject)", [result isEqualToObject:@"function TestObject() {\n    [native code]\n}"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TiValue* value = [TiValue valueWithObject:[TestObject class] inContext:context];
        checkResult(@"[value toObject] == [TestObject class]", [value toObject] == [TestObject class]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"TestObject"] = [TestObject class];
        TiValue *result = [context evaluateScript:@"TestObject.parentTest()"];
        checkResult(@"TestObject.parentTest()", [result isEqualToObject:@"TestObject"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject* testObject = [TestObject testObject];
        context[@"testObjectA"] = testObject;
        context[@"testObjectB"] = testObject;
        TiValue *result = [context evaluateScript:@"testObjectA == testObjectB"];
        checkResult(@"testObjectA == testObjectB", [result isBoolean] && [result toBool]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject* testObject = [TestObject testObject];
        context[@"testObject"] = testObject;
        testObject.point = (CGPoint){3,4};
        TiValue *result = [context evaluateScript:@"var result = JSON.stringify(testObject.point); testObject.point = {x:12,y:14}; result"];
        checkResult(@"testObject.point - result", [result isEqualToObject:@"{\"x\":3,\"y\":4}"]);
        checkResult(@"testObject.point - {x:12,y:14}", testObject.point.x == 12 && testObject.point.y == 14);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject* testObject = [TestObject testObject];
        testObject.six = 6;
        context[@"testObject"] = testObject;
        context[@"mul"] = ^(int x, int y){ return x * y; };
        TiValue *result = [context evaluateScript:@"mul(testObject.six, 7)"];
        checkResult(@"mul(testObject.six, 7)", [result isNumber] && [result toInt32] == 42);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject* testObject = [TestObject testObject];
        context[@"testObject"] = testObject;
        context[@"testObject"][@"variable"] = @4;
        [context evaluateScript:@"++testObject.variable"];
        checkResult(@"++testObject.variable", testObject.variable == 5);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"point"] = @{ @"x":@6, @"y":@7 };
        TiValue *result = [context evaluateScript:@"point.x + ',' + point.y"];
        checkResult(@"point.x + ',' + point.y", [result isEqualToObject:@"6,7"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"point"] = @{ @"x":@6, @"y":@7 };
        TiValue *result = [context evaluateScript:@"point.x + ',' + point.y"];
        checkResult(@"point.x + ',' + point.y", [result isEqualToObject:@"6,7"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject* testObject = [TestObject testObject];
        context[@"testObject"] = testObject;
        TiValue *result = [context evaluateScript:@"testObject.getString()"];
        checkResult(@"testObject.getString()", [result isString] && [result toInt32] == 42);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject* testObject = [TestObject testObject];
        context[@"testObject"] = testObject;
        TiValue *result = [context evaluateScript:@"testObject.testArgumentTypes(101,0.5,true,'foo',666,[false,'bar',false],{x:'baz'})"];
        checkResult(@"testObject.testArgumentTypes", [result isEqualToObject:@"101,0.5,1,foo,666,bar,baz"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject* testObject = [TestObject testObject];
        context[@"testObject"] = testObject;
        TiValue *result = [context evaluateScript:@"testObject.getString.call(testObject)"];
        checkResult(@"testObject.getString.call(testObject)", [result isString] && [result toInt32] == 42);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject* testObject = [TestObject testObject];
        context[@"testObject"] = testObject;
        checkResult(@"testObject.getString.call({}) pre", !context.exception);
        [context evaluateScript:@"testObject.getString.call({})"];
        checkResult(@"testObject.getString.call({}) post", context.exception);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject* testObject = [TestObject testObject];
        context[@"testObject"] = testObject;
        TiValue *result = [context evaluateScript:@"var result = 0; testObject.callback(function(x){ result = x; }); result"];
        checkResult(@"testObject.callback", [result isNumber] && [result toInt32] == 42);
        result = [context evaluateScript:@"testObject.bogusCallback"];
        checkResult(@"testObject.bogusCallback == undefined", [result isUndefined]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject *testObject = [TestObject testObject];
        context[@"testObject"] = testObject;
        TiValue *result = [context evaluateScript:@"Function.prototype.toString.call(testObject.callback)"];
        checkResult(@"Function.prototype.toString", !context.exception && ![result isUndefined]);
    }

    @autoreleasepool {
        TiContext *context1 = [[TiContext alloc] init];
        TiContext *context2 = [[TiContext alloc] initWithVirtualMachine:context1.virtualMachine];
        TiValue *value = [TiValue valueWithDouble:42 inContext:context2];
        context1[@"passValueBetweenContexts"] = value;
        TiValue *result = [context1 evaluateScript:@"passValueBetweenContexts"];
        checkResult(@"[value isEqualToObject:result]", [value isEqualToObject:result]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"handleTheDictionary"] = ^(NSDictionary *dict) {
            NSDictionary *expectedDict = @{
                @"foo" : [NSNumber numberWithInt:1],
                @"bar" : @{
                    @"baz": [NSNumber numberWithInt:2]
                }
            };
            checkResult(@"recursively convert nested dictionaries", [dict isEqualToDictionary:expectedDict]);
        };
        [context evaluateScript:@"var myDict = { \
            'foo': 1, \
            'bar': {'baz': 2} \
        }; \
        handleTheDictionary(myDict);"];

        context[@"handleTheArray"] = ^(NSArray *array) {
            NSArray *expectedArray = @[@"foo", @"bar", @[@"baz"]];
            checkResult(@"recursively convert nested arrays", [array isEqualToArray:expectedArray]);
        };
        [context evaluateScript:@"var myArray = ['foo', 'bar', ['baz']]; handleTheArray(myArray);"];
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject *testObject = [TestObject testObject];
        @autoreleasepool {
            context[@"testObject"] = testObject;
            [context evaluateScript:@"var constructor = Object.getPrototypeOf(testObject).constructor; constructor.prototype = undefined;"];
            [context evaluateScript:@"testObject = undefined"];
        }
        
        JSSynchronousGarbageCollectForDebugging([context TiGlobalContextRef]);

        @autoreleasepool {
            context[@"testObject"] = testObject;
        }
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TextXYZ *testXYZ = [[TextXYZ alloc] init];

        @autoreleasepool {
            context[@"testXYZ"] = testXYZ;

            [context evaluateScript:@" \
                didClick = false; \
                testXYZ.onclick = function() { \
                    didClick = true; \
                }; \
                 \
                testXYZ.weakOnclick = function() { \
                    return 'foo'; \
                }; \
            "];
        }

        @autoreleasepool {
            [testXYZ click];
            TiValue *result = [context evaluateScript:@"didClick"];
            checkResult(@"Event handler onclick", [result toBool]);
        }

        JSSynchronousGarbageCollectForDebugging([context TiGlobalContextRef]);

        @autoreleasepool {
            TiValue *result = [context evaluateScript:@"testXYZ.onclick"];
            checkResult(@"onclick still around after GC", !([result isNull] || [result isUndefined]));
        }


        @autoreleasepool {
            TiValue *result = [context evaluateScript:@"testXYZ.weakOnclick"];
            checkResult(@"weakOnclick not around after GC", [result isNull] || [result isUndefined]);
        }

        @autoreleasepool {
            [context evaluateScript:@" \
                didClick = false; \
                testXYZ = null; \
            "];
        }

        JSSynchronousGarbageCollectForDebugging([context TiGlobalContextRef]);

        @autoreleasepool {
            [testXYZ click];
            TiValue *result = [context evaluateScript:@"didClick"];
            checkResult(@"Event handler onclick doesn't fire", ![result toBool]);
        }
    }

    @autoreleasepool {
        TiVirtualMachine *vm = [[TiVirtualMachine alloc] init];
        TestObject *testObject = [TestObject testObject];
        TiManagedValue *weakValue;
        @autoreleasepool {
            TiContext *context = [[TiContext alloc] initWithVirtualMachine:vm];
            context[@"testObject"] = testObject;
            weakValue = [[TiManagedValue alloc] initWithValue:context[@"testObject"]];
        }

        @autoreleasepool {
            TiContext *context = [[TiContext alloc] initWithVirtualMachine:vm];
            context[@"testObject"] = testObject;
            JSSynchronousGarbageCollectForDebugging([context TiGlobalContextRef]);
            checkResult(@"weak value == nil", ![weakValue value]);
            checkResult(@"root is still alive", ![context[@"testObject"] isUndefined]);
        }
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TinyDOMNode *root = [[TinyDOMNode alloc] initWithVirtualMachine:context.virtualMachine];
        TinyDOMNode *lastNode = root;
        for (NSUInteger i = 0; i < 3; i++) {
            TinyDOMNode *newNode = [[TinyDOMNode alloc] initWithVirtualMachine:context.virtualMachine];
            [lastNode appendChild:newNode];
            lastNode = newNode;
        }

        @autoreleasepool {
            context[@"root"] = root;
            context[@"getLastNodeInChain"] = ^(TinyDOMNode *head){
                TinyDOMNode *lastNode = nil;
                while (head) {
                    lastNode = head;
                    head = [lastNode childAtIndex:0];
                }
                return lastNode;
            };
            [context evaluateScript:@"getLastNodeInChain(root).myCustomProperty = 42;"];
        }

        JSSynchronousGarbageCollectForDebugging([context TiGlobalContextRef]);

        TiValue *myCustomProperty = [context evaluateScript:@"getLastNodeInChain(root).myCustomProperty"];
        checkResult(@"My custom property == 42", [myCustomProperty isNumber] && [myCustomProperty toInt32] == 42);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TinyDOMNode *root = [[TinyDOMNode alloc] initWithVirtualMachine:context.virtualMachine];
        TinyDOMNode *lastNode = root;
        for (NSUInteger i = 0; i < 3; i++) {
            TinyDOMNode *newNode = [[TinyDOMNode alloc] initWithVirtualMachine:context.virtualMachine];
            [lastNode appendChild:newNode];
            lastNode = newNode;
        }

        @autoreleasepool {
            context[@"root"] = root;
            context[@"getLastNodeInChain"] = ^(TinyDOMNode *head){
                TinyDOMNode *lastNode = nil;
                while (head) {
                    lastNode = head;
                    head = [lastNode childAtIndex:0];
                }
                return lastNode;
            };
            [context evaluateScript:@"getLastNodeInChain(root).myCustomProperty = 42;"];

            [root appendChild:[root childAtIndex:0]];
            [root removeChildAtIndex:0];
        }

        JSSynchronousGarbageCollectForDebugging([context TiGlobalContextRef]);

        TiValue *myCustomProperty = [context evaluateScript:@"getLastNodeInChain(root).myCustomProperty"];
        checkResult(@"duplicate calls to addManagedReference don't cause things to die", [myCustomProperty isNumber] && [myCustomProperty toInt32] == 42);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TiValue *o = [TiValue valueWithNewObjectInContext:context];
        o[@"foo"] = @"foo";
        JSSynchronousGarbageCollectForDebugging([context TiGlobalContextRef]);

        checkResult(@"TiValue correctly protected its internal value", [[o[@"foo"] toString] isEqualToString:@"foo"]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        TestObject *testObject = [TestObject testObject];
        context[@"testObject"] = testObject;
        [context evaluateScript:@"testObject.__lookupGetter__('variable').call({})"];
        checkResult(@"Make sure we throw an exception when calling getter on incorrect |this|", context.exception);
    }

    @autoreleasepool {
        TestObject *testObject = [TestObject testObject];
        TiManagedValue *managedTestObject;
        @autoreleasepool {
            TiContext *context = [[TiContext alloc] init];
            context[@"testObject"] = testObject;
            managedTestObject = [TiManagedValue managedValueWithValue:context[@"testObject"]];
            [context.virtualMachine addManagedReference:managedTestObject withOwner:testObject];
        }
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"MyClass"] = ^{
            TiValue *newThis = [TiValue valueWithNewObjectInContext:[TiContext currentContext]];
            TiGlobalContextRef contextRef = [[TiContext currentContext] TiGlobalContextRef];
            TiObjectRef newThisRef = TiValueToObject(contextRef, [newThis TiValueRef], NULL);
            TiObjectSetPrototype(contextRef, newThisRef, [[TiContext currentContext][@"MyClass"][@"prototype"] TiValueRef]);
            return newThis;
        };

        context[@"MyOtherClass"] = ^{
            TiValue *newThis = [TiValue valueWithNewObjectInContext:[TiContext currentContext]];
            TiGlobalContextRef contextRef = [[TiContext currentContext] TiGlobalContextRef];
            TiObjectRef newThisRef = TiValueToObject(contextRef, [newThis TiValueRef], NULL);
            TiObjectSetPrototype(contextRef, newThisRef, [[TiContext currentContext][@"MyOtherClass"][@"prototype"] TiValueRef]);
            return newThis;
        };

        context.exceptionHandler = ^(TiContext *context, TiValue *exception) {
            NSLog(@"EXCEPTION: %@", [exception toString]);
            context.exception = nil;
        };

        TiValue *constructor1 = context[@"MyClass"];
        TiValue *constructor2 = context[@"MyOtherClass"];

        TiValue *value1 = [context evaluateScript:@"new MyClass()"];
        checkResult(@"value1 instanceof MyClass", [value1 isInstanceOf:constructor1]);
        checkResult(@"!(value1 instanceof MyOtherClass)", ![value1 isInstanceOf:constructor2]);
        checkResult(@"MyClass.prototype.constructor === MyClass", [[context evaluateScript:@"MyClass.prototype.constructor === MyClass"] toBool]);
        checkResult(@"MyClass instanceof Function", [[context evaluateScript:@"MyClass instanceof Function"] toBool]);

        TiValue *value2 = [context evaluateScript:@"new MyOtherClass()"];
        checkResult(@"value2 instanceof MyOtherClass", [value2 isInstanceOf:constructor2]);
        checkResult(@"!(value2 instanceof MyClass)", ![value2 isInstanceOf:constructor1]);
        checkResult(@"MyOtherClass.prototype.constructor === MyOtherClass", [[context evaluateScript:@"MyOtherClass.prototype.constructor === MyOtherClass"] toBool]);
        checkResult(@"MyOtherClass instanceof Function", [[context evaluateScript:@"MyOtherClass instanceof Function"] toBool]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"MyClass"] = ^{
            NSLog(@"I'm intentionally not returning anything.");
        };
        TiValue *result = [context evaluateScript:@"new MyClass()"];
        checkResult(@"result === undefined", [result isUndefined]);
        checkResult(@"exception.message is correct'", context.exception 
            && [@"Objective-C blocks called as constructors must return an object." isEqualToString:[context.exception[@"message"] toString]]);
    }

    @autoreleasepool {
        checkResult(@"[TiContext currentThis] == nil outside of callback", ![TiContext currentThis]);
        checkResult(@"[TiContext currentArguments] == nil outside of callback", ![TiContext currentArguments]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"TestObject"] = [TestObject class];
        TiValue *testObject = [context evaluateScript:@"(new TestObject())"];
        checkResult(@"testObject instanceof TestObject", [testObject isInstanceOf:context[@"TestObject"]]);

        context[@"TextXYZ"] = [TextXYZ class];
        TiValue *textObject = [context evaluateScript:@"(new TextXYZ(\"Called TextXYZ constructor!\"))"];
        checkResult(@"textObject instanceof TextXYZ", [textObject isInstanceOf:context[@"TextXYZ"]]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"ClassA"] = [ClassA class];
        context[@"ClassB"] = [ClassB class];
        context[@"ClassC"] = [ClassC class]; // Should print error message about too many inits found.
        context[@"ClassCPrime"] = [ClassCPrime class]; // Ditto.

        TiValue *a = [context evaluateScript:@"(new ClassA(42))"];
        checkResult(@"a instanceof ClassA", [a isInstanceOf:context[@"ClassA"]]);
        checkResult(@"a.initialize() is callable", [[a invokeMethod:@"initialize" withArguments:@[]] toInt32] == 42);

        TiValue *b = [context evaluateScript:@"(new ClassB(42, 53))"];
        checkResult(@"b instanceof ClassB", [b isInstanceOf:context[@"ClassB"]]);

        TiValue *canConstructClassC = [context evaluateScript:@"(function() { \
            try { \
                (new ClassC(1, 2)); \
                return true; \
            } catch(e) { \
                return false; \
            } \
        })()"];
        checkResult(@"shouldn't be able to construct ClassC", ![canConstructClassC toBool]);
        TiValue *canConstructClassCPrime = [context evaluateScript:@"(function() { \
            try { \
                (new ClassCPrime(1)); \
                return true; \
            } catch(e) { \
                return false; \
            } \
        })()"];
        checkResult(@"shouldn't be able to construct ClassCPrime", ![canConstructClassCPrime toBool]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        context[@"ClassD"] = [ClassD class];
        context[@"ClassE"] = [ClassE class];
       
        TiValue *d = [context evaluateScript:@"(new ClassD())"];
        checkResult(@"Returning instance of ClassE from ClassD's init has correct class", [d isInstanceOf:context[@"ClassE"]]);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        @autoreleasepool {
            EvilAllocationObject *evilObject = [[EvilAllocationObject alloc] initWithContext:context];
            context[@"evilObject"] = evilObject;
            context[@"evilObject"] = nil;
        }
        JSSynchronousGarbageCollectForDebugging([context TiGlobalContextRef]);
        checkResult(@"EvilAllocationObject was successfully dealloced without crashing", evilAllocationObjectWasDealloced);
    }

    @autoreleasepool {
        TiContext *context = [[TiContext alloc] init];
        checkResult(@"default context.name is nil", context.name == nil);
        NSString *name1 = @"Name1";
        NSString *name2 = @"Name2";
        context.name = name1;
        NSString *fetchedName1 = context.name;
        context.name = name2;
        NSString *fetchedName2 = context.name;
        checkResult(@"fetched context.name was expected", [fetchedName1 isEqualToString:name1]);
        checkResult(@"fetched context.name was expected", [fetchedName2 isEqualToString:name2]);
        checkResult(@"fetched context.name was expected", ![fetchedName1 isEqualToString:fetchedName2]);
    }

    currentThisInsideBlockGetterTest();
}

#else

void testObjectiveCAPI()
{
}

#endif // JSC_OBJC_API_ENABLED
