/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2014 by Appcelerator, Inc.
 */

/*
 *  Copyright (C) 2006, 2009, 2011 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef WTF_Forward_h
#define WTF_Forward_h

#include <stddef.h>

namespace WTI {

template<typename T> class Function;
template<typename T> class OwnPtr;
template<typename T> class PassOwnPtr;
template<typename T> class PassRef;
template<typename T> class PassRefPtr;
template<typename T> class RefPtr;
template<typename T> class Ref;
template<typename T> class StringBuffer;

template<typename T, size_t inlineCapacity, typename OverflowHandler> class Vector;

class AtomicString;
class AtomicStringImpl;
class BinarySemaphore;
class CString;
class Decoder;
class Encoder;
class FunctionDispatcher;
class PrintStream;
class String;
class StringBuilder;
class StringImpl;

}

using WTI::AtomicString;
using WTI::AtomicStringImpl;
using WTI::BinarySemaphore;
using WTI::CString;
using WTI::Decoder;
using WTI::Encoder;
using WTI::Function;
using WTI::FunctionDispatcher;
using WTI::OwnPtr;
using WTI::PassOwnPtr;
using WTI::PassRef;
using WTI::PassRefPtr;
using WTI::PrintStream;
using WTI::Ref;
using WTI::RefPtr;
using WTI::String;
using WTI::StringBuffer;
using WTI::StringBuilder;
using WTI::StringImpl;
using WTI::Vector;

#endif // WTF_Forward_h
