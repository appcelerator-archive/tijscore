/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2013 Apple Inc. All Rights Reserved.
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

#import "config.h"
#import "RemoteInspectorXPCConnection.h"

#if ENABLE(REMOTE_INSPECTOR)

#import <Foundation/Foundation.h>
#import <wtf/Assertions.h>

#if __has_include(<CoreFoundation/CFXPCBridge.h>)
#import <CoreFoundation/CFXPCBridge.h>
#else
extern "C" xpc_object_t _CFXPCCreateXPCMessageWithCFObject(CFTypeRef);
extern "C" CFTypeRef _CFXPCCreateCFObjectFromXPCMessage(xpc_object_t);
#endif

namespace Inspector {

// Constants private to this file for message serialization on both ends.
#define RemoteInspectorXPCConnectionMessageNameKey @"messageName"
#define RemoteInspectorXPCConnectionUserInfoKey @"userInfo"
#define RemoteInspectorXPCConnectionSerializedMessageKey "msgData"

RemoteInspectorXPCConnection::RemoteInspectorXPCConnection(xpc_connection_t connection, Client* client)
    : m_connection(connection)
    , m_queue(dispatch_queue_create("com.apple.JavaScriptCore.remote-inspector-xpc-connection", DISPATCH_QUEUE_SERIAL))
    , m_client(client)
{
    xpc_retain(m_connection);
    xpc_connection_set_target_queue(m_connection, m_queue);
    RemoteInspectorXPCConnection* weakThis = this;
    xpc_connection_set_event_handler(m_connection, ^(xpc_object_t object) {
        weakThis->handleEvent(object);
    });
    xpc_connection_resume(m_connection);
}

RemoteInspectorXPCConnection::~RemoteInspectorXPCConnection()
{
    ASSERT(!m_client);
    ASSERT(!m_connection);
}

void RemoteInspectorXPCConnection::close()
{
    if (!m_connection)
        return;

    xpc_connection_cancel(m_connection);
    xpc_release(m_connection);
    m_connection = NULL;

    dispatch_release(m_queue);
    m_queue = NULL;

    m_client = nullptr;
}

NSDictionary *RemoteInspectorXPCConnection::deserializeMessage(xpc_object_t object)
{
    if (xpc_get_type(object) != XPC_TYPE_DICTIONARY)
        return nil;

    xpc_object_t xpcDictionary = xpc_dictionary_get_value(object, RemoteInspectorXPCConnectionSerializedMessageKey);
    if (!xpcDictionary || xpc_get_type(xpcDictionary) != XPC_TYPE_DICTIONARY) {
        if (m_client)
            m_client->xpcConnectionUnhandledMessage(this, object);
        return nil;
    }

    NSDictionary *dictionary = static_cast<NSDictionary *>(_CFXPCCreateCFObjectFromXPCMessage(xpcDictionary));
    ASSERT_WITH_MESSAGE(dictionary, "Unable to deserialize xpc message");
    return [dictionary autorelease];
}

void RemoteInspectorXPCConnection::handleEvent(xpc_object_t object)
{
    if (!m_connection)
        return;

    if (xpc_get_type(object) == XPC_TYPE_ERROR) {
        if (m_client)
            m_client->xpcConnectionFailed(this);
        return;
    }

    NSDictionary *dataDictionary = deserializeMessage(object);
    if (!dataDictionary)
        return;

    NSString *message = [dataDictionary objectForKey:RemoteInspectorXPCConnectionMessageNameKey];
    NSDictionary *userInfo = [dataDictionary objectForKey:RemoteInspectorXPCConnectionUserInfoKey];
    if (m_client)
        m_client->xpcConnectionReceivedMessage(this, message, userInfo);
}

void RemoteInspectorXPCConnection::sendMessage(NSString *messageName, NSDictionary *userInfo)
{
    if (!m_connection)
        return;

    NSMutableDictionary *dictionary = [NSMutableDictionary dictionaryWithObject:messageName forKey:RemoteInspectorXPCConnectionMessageNameKey];
    if (userInfo)
        [dictionary setObject:userInfo forKey:RemoteInspectorXPCConnectionUserInfoKey];

    xpc_object_t xpcDictionary = _CFXPCCreateXPCMessageWithCFObject((CFDictionaryRef)dictionary);
    ASSERT_WITH_MESSAGE(xpcDictionary && xpc_get_type(xpcDictionary) == XPC_TYPE_DICTIONARY, "Unable to serialize xpc message");
    if (!xpcDictionary)
        return;

    xpc_object_t msg = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_value(msg, RemoteInspectorXPCConnectionSerializedMessageKey, xpcDictionary);
    xpc_release(xpcDictionary);

    xpc_connection_send_message(m_connection, msg);

    xpc_release(msg);
}

} // namespace Inspector

#endif // ENABLE(REMOTE_INSPECTOR)
