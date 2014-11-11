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

#if ENABLE(REMOTE_INSPECTOR)

#ifndef RemoteInspector_h
#define RemoteInspector_h

#import "RemoteInspectorDebuggableConnection.h"
#import "RemoteInspectorXPCConnection.h"
#import <wtf/Forward.h>
#import <wtf/HashMap.h>
#import <wtf/NeverDestroyed.h>
#import <wtf/Threading.h>

OBJC_CLASS NSString;
OBJC_CLASS NSDictionary;

namespace Inspector {

class RemoteInspectorDebuggable;
struct RemoteInspectorDebuggableInfo;

class JS_EXPORT_PRIVATE RemoteInspector FINAL : public RemoteInspectorXPCConnection::Client {
public:
    static RemoteInspector& shared();
    friend class NeverDestroyed<RemoteInspector>;

    void registerDebuggable(RemoteInspectorDebuggable*);
    void unregisterDebuggable(RemoteInspectorDebuggable*);
    void updateDebuggable(RemoteInspectorDebuggable*);
    void sendMessageToRemoteFrontend(unsigned identifier, const String& message);

    bool enabled() const { return m_enabled; }
    bool hasActiveDebugSession() const { return m_hasActiveDebugSession; }

    void start();
    void stop();

private:
    RemoteInspector();

    unsigned nextAvailableIdentifier();

    void setupXPCConnectionIfNeeded();

    NSDictionary *listingForDebuggable(const RemoteInspectorDebuggableInfo&) const;
    void pushListingNow();
    void pushListingSoon();

    void updateHasActiveDebugSession();

    virtual void xpcConnectionReceivedMessage(RemoteInspectorXPCConnection*, NSString *messageName, NSDictionary *userInfo) OVERRIDE;
    virtual void xpcConnectionFailed(RemoteInspectorXPCConnection*) OVERRIDE;
    virtual void xpcConnectionUnhandledMessage(RemoteInspectorXPCConnection*, xpc_object_t) OVERRIDE;

    void receivedSetupMessage(NSDictionary *userInfo);
    void receivedDataMessage(NSDictionary *userInfo);
    void receivedDidCloseMessage(NSDictionary *userInfo);
    void receivedGetListingMessage(NSDictionary *userInfo);
    void receivedIndicateMessage(NSDictionary *userInfo);
    void receivedConnectionDiedMessage(NSDictionary *userInfo);

    // Debuggables can be registered from any thread at any time.
    // Any debuggable can send messages over the XPC connection.
    // So lock access to all maps and state as they can change
    // from any thread.
    Mutex m_lock;

    HashMap<unsigned, std::pair<RemoteInspectorDebuggable*, RemoteInspectorDebuggableInfo>> m_debuggableMap;
    HashMap<unsigned, RefPtr<RemoteInspectorDebuggableConnection>> m_connectionMap;
    std::unique_ptr<RemoteInspectorXPCConnection> m_xpcConnection;
    unsigned m_nextAvailableIdentifier;
    int m_notifyToken;
    bool m_enabled;
    bool m_hasActiveDebugSession;
    bool m_pushScheduled;
};

} // namespace Inspector

#endif // ENABLE(REMOTE_INSPECTOR)

#endif // WebInspectorServer_h
