/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009-2012 by Appcelerator, Inc.
 */

/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
#include "LiteralParser.h"

#include "TiArray.h"
#include "TiString.h"
#include "Lexer.h"
#include "UStringBuilder.h"
#include <wtf/ASCIICType.h>
#include <wtf/dtoa.h>

namespace TI {

static inline bool isJSONWhiteSpace(const UChar& c)
{
    // The JSON RFC 4627 defines a list of allowed characters to be considered
    // insignificant white space: http://www.ietf.org/rfc/rfc4627.txt (2. JSON Grammar).
    return c == ' ' || c == 0x9 || c == 0xA || c == 0xD;
}

bool LiteralParser::tryJSONPParse(Vector<JSONPData>& results, bool needsFullSourceInfo)
{
    if (m_lexer.next() != TokIdentifier)
        return false;
    do {
        Vector<JSONPPathEntry> path;
        // Unguarded next to start off the lexer
        Identifier name = Identifier(m_exec, m_lexer.currentToken().start, m_lexer.currentToken().end - m_lexer.currentToken().start);
        JSONPPathEntry entry;
        if (name == m_exec->globalData().propertyNames->varKeyword) {
            if (m_lexer.next() != TokIdentifier)
                return false;
            entry.m_type = JSONPPathEntryTypeDeclare;
            entry.m_pathEntryName = Identifier(m_exec, m_lexer.currentToken().start, m_lexer.currentToken().end - m_lexer.currentToken().start);
            path.append(entry);
        } else {
            entry.m_type = JSONPPathEntryTypeDot;
            entry.m_pathEntryName = Identifier(m_exec, m_lexer.currentToken().start, m_lexer.currentToken().end - m_lexer.currentToken().start);
            path.append(entry);
        }
        if (m_exec->globalData().lexer->isKeyword(entry.m_pathEntryName))
            return false;
        TokenType tokenType = m_lexer.next();
        while (tokenType != TokAssign) {
            switch (tokenType) {
            case TokLBracket: {
                entry.m_type = JSONPPathEntryTypeLookup;
                if (m_lexer.next() != TokNumber)
                    return false;
                double doubleIndex = m_lexer.currentToken().numberToken;
                int index = (int)doubleIndex;
                if (index != doubleIndex || index < 0)
                    return false;
                entry.m_pathIndex = index;
                if (m_lexer.next() != TokRBracket)
                    return false;
                break;
            }
            case TokDot: {
                entry.m_type = JSONPPathEntryTypeDot;
                if (m_lexer.next() != TokIdentifier)
                    return false;
                entry.m_pathEntryName = Identifier(m_exec, m_lexer.currentToken().start, m_lexer.currentToken().end - m_lexer.currentToken().start);
                break;
            }
            case TokLParen: {
                if (path.last().m_type != JSONPPathEntryTypeDot || needsFullSourceInfo)
                    return false;
                path.last().m_type = JSONPPathEntryTypeCall;
                entry = path.last();
                goto startJSON;
            }
            default:
                return false;
            }
            path.append(entry);
            tokenType = m_lexer.next();
        }
    startJSON:
        m_lexer.next();
        results.append(JSONPData());
        results.last().m_value.set(m_exec->globalData(), parse(StartParseExpression));
        if (!results.last().m_value)
            return false;
        results.last().m_path.swap(path);
        if (entry.m_type == JSONPPathEntryTypeCall) {
            if (m_lexer.currentToken().type != TokRParen)
                return false;
            m_lexer.next();
        }
        if (m_lexer.currentToken().type != TokSemi)
            break;
        m_lexer.next();
    } while (m_lexer.currentToken().type == TokIdentifier);
    return m_lexer.currentToken().type == TokEnd;
}
    
ALWAYS_INLINE const Identifier LiteralParser::makeIdentifier(const UChar* characters, size_t length)
{
    if (!length)
        return m_exec->globalData().propertyNames->emptyIdentifier;
    if (characters[0] >= MaximumCachableCharacter)
        return Identifier(&m_exec->globalData(), characters, length);

    if (length == 1) {
        if (!m_shortIdentifiers[characters[0]].isNull())
            return m_shortIdentifiers[characters[0]];
        m_shortIdentifiers[characters[0]] = Identifier(&m_exec->globalData(), characters, length);
        return m_shortIdentifiers[characters[0]];
    }
    if (!m_recentIdentifiers[characters[0]].isNull() && Identifier::equal(m_recentIdentifiers[characters[0]].impl(), characters, length))
        return m_recentIdentifiers[characters[0]];
    m_recentIdentifiers[characters[0]] = Identifier(&m_exec->globalData(), characters, length);
    return m_recentIdentifiers[characters[0]];
}

template <LiteralParser::ParserMode mode> LiteralParser::TokenType LiteralParser::Lexer::lex(LiteralParserToken& token)
{
    while (m_ptr < m_end && isJSONWhiteSpace(*m_ptr))
        ++m_ptr;

    ASSERT(m_ptr <= m_end);
    if (m_ptr >= m_end) {
        token.type = TokEnd;
        token.start = token.end = m_ptr;
        return TokEnd;
    }
    token.type = TokError;
    token.start = m_ptr;
    switch (*m_ptr) {
        case '[':
            token.type = TokLBracket;
            token.end = ++m_ptr;
            return TokLBracket;
        case ']':
            token.type = TokRBracket;
            token.end = ++m_ptr;
            return TokRBracket;
        case '(':
            token.type = TokLParen;
            token.end = ++m_ptr;
            return TokLParen;
        case ')':
            token.type = TokRParen;
            token.end = ++m_ptr;
            return TokRParen;
        case '{':
            token.type = TokLBrace;
            token.end = ++m_ptr;
            return TokLBrace;
        case '}':
            token.type = TokRBrace;
            token.end = ++m_ptr;
            return TokRBrace;
        case ',':
            token.type = TokComma;
            token.end = ++m_ptr;
            return TokComma;
        case ':':
            token.type = TokColon;
            token.end = ++m_ptr;
            return TokColon;
        case '"':
            return lexString<mode, '"'>(token);
        case 't':
            if (m_end - m_ptr >= 4 && m_ptr[1] == 'r' && m_ptr[2] == 'u' && m_ptr[3] == 'e') {
                m_ptr += 4;
                token.type = TokTrue;
                token.end = m_ptr;
                return TokTrue;
            }
            break;
        case 'f':
            if (m_end - m_ptr >= 5 && m_ptr[1] == 'a' && m_ptr[2] == 'l' && m_ptr[3] == 's' && m_ptr[4] == 'e') {
                m_ptr += 5;
                token.type = TokFalse;
                token.end = m_ptr;
                return TokFalse;
            }
            break;
        case 'n':
            if (m_end - m_ptr >= 4 && m_ptr[1] == 'u' && m_ptr[2] == 'l' && m_ptr[3] == 'l') {
                m_ptr += 4;
                token.type = TokNull;
                token.end = m_ptr;
                return TokNull;
            }
            break;
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return lexNumber(token);
    }
    if (m_ptr < m_end) {
        if (*m_ptr == '.') {
            token.type = TokDot;
            token.end = ++m_ptr;
            return TokDot;
        }
        if (*m_ptr == '=') {
            token.type = TokAssign;
            token.end = ++m_ptr;
            return TokAssign;
        }
        if (*m_ptr == ';') {
            token.type = TokSemi;
            token.end = ++m_ptr;
            return TokAssign;
        }
        if (isASCIIAlpha(*m_ptr) || *m_ptr == '_' || *m_ptr == '$') {
            while (m_ptr < m_end && (isASCIIAlphanumeric(*m_ptr) || *m_ptr == '_' || *m_ptr == '$'))
                m_ptr++;
            token.stringToken = token.start;
            token.stringLength = m_ptr - token.start;
            token.type = TokIdentifier;
            token.end = m_ptr;
            return TokIdentifier;
        }
        if (*m_ptr == '\'') {
            if (mode == StrictJSON)
                return TokError;
            return lexString<mode, '\''>(token);
        }
    }
    return TokError;
}

LiteralParser::TokenType LiteralParser::Lexer::next()
{
    if (m_mode == NonStrictJSON)
        return lex<NonStrictJSON>(m_currentToken);
    if (m_mode == JSONP)
        return lex<JSONP>(m_currentToken);
    return lex<StrictJSON>(m_currentToken);
}

template <LiteralParser::ParserMode mode, UChar terminator> static inline bool isSafeStringCharacter(UChar c)
{
    return (c >= ' ' && (mode == LiteralParser::StrictJSON || c <= 0xff) && c != '\\' && c != terminator) || c == '\t';
}

// "inline" is required here to help WINSCW compiler resolve specialized argument in templated functions.
template <LiteralParser::ParserMode mode, UChar terminator> inline LiteralParser::TokenType LiteralParser::Lexer::lexString(LiteralParserToken& token)
{
    ++m_ptr;
    const UChar* runStart = m_ptr;
    UStringBuilder builder;
    do {
        runStart = m_ptr;
        while (m_ptr < m_end && isSafeStringCharacter<mode, terminator>(*m_ptr))
            ++m_ptr;
        if (builder.length())
            builder.append(runStart, m_ptr - runStart);
        if ((mode != NonStrictJSON) && m_ptr < m_end && *m_ptr == '\\') {
            if (builder.isEmpty() && runStart < m_ptr)
                builder.append(runStart, m_ptr - runStart);
            ++m_ptr;
            if (m_ptr >= m_end)
                return TokError;
            switch (*m_ptr) {
                case '"':
                    builder.append('"');
                    m_ptr++;
                    break;
                case '\\':
                    builder.append('\\');
                    m_ptr++;
                    break;
                case '/':
                    builder.append('/');
                    m_ptr++;
                    break;
                case 'b':
                    builder.append('\b');
                    m_ptr++;
                    break;
                case 'f':
                    builder.append('\f');
                    m_ptr++;
                    break;
                case 'n':
                    builder.append('\n');
                    m_ptr++;
                    break;
                case 'r':
                    builder.append('\r');
                    m_ptr++;
                    break;
                case 't':
                    builder.append('\t');
                    m_ptr++;
                    break;

                case 'u':
                    if ((m_end - m_ptr) < 5) // uNNNN == 5 characters
                        return TokError;
                    for (int i = 1; i < 5; i++) {
                        if (!isASCIIHexDigit(m_ptr[i]))
                            return TokError;
                    }
                    builder.append(TI::Lexer::convertUnicode(m_ptr[1], m_ptr[2], m_ptr[3], m_ptr[4]));
                    m_ptr += 5;
                    break;

                default:
                    if (*m_ptr == '\'' && mode != StrictJSON) {
                        builder.append('\'');
                        m_ptr++;
                        break;
                    }
                    return TokError;
            }
        }
    } while ((mode != NonStrictJSON) && m_ptr != runStart && (m_ptr < m_end) && *m_ptr != terminator);

    if (m_ptr >= m_end || *m_ptr != terminator)
        return TokError;

    if (builder.isEmpty()) {
        token.stringBuffer = UString();
        token.stringToken = runStart;
        token.stringLength = m_ptr - runStart;
    } else {
        token.stringBuffer = builder.toUString();
        token.stringToken = token.stringBuffer.characters();
        token.stringLength = token.stringBuffer.length();
    }
    token.type = TokString;
    token.end = ++m_ptr;
    return TokString;
}

LiteralParser::TokenType LiteralParser::Lexer::lexNumber(LiteralParserToken& token)
{
    // ES5 and json.org define numbers as
    // number
    //     int
    //     int frac? exp?
    //
    // int
    //     -? 0
    //     -? digit1-9 digits?
    //
    // digits
    //     digit digits?
    //
    // -?(0 | [1-9][0-9]*) ('.' [0-9]+)? ([eE][+-]? [0-9]+)?

    if (m_ptr < m_end && *m_ptr == '-') // -?
        ++m_ptr;
    
    // (0 | [1-9][0-9]*)
    if (m_ptr < m_end && *m_ptr == '0') // 0
        ++m_ptr;
    else if (m_ptr < m_end && *m_ptr >= '1' && *m_ptr <= '9') { // [1-9]
        ++m_ptr;
        // [0-9]*
        while (m_ptr < m_end && isASCIIDigit(*m_ptr))
            ++m_ptr;
    } else
        return TokError;

    // ('.' [0-9]+)?
    if (m_ptr < m_end && *m_ptr == '.') {
        ++m_ptr;
        // [0-9]+
        if (m_ptr >= m_end || !isASCIIDigit(*m_ptr))
            return TokError;

        ++m_ptr;
        while (m_ptr < m_end && isASCIIDigit(*m_ptr))
            ++m_ptr;
    } else if (m_ptr < m_end && (*m_ptr != 'e' && *m_ptr != 'E') && (m_ptr - token.start) < 10) {
        int result = 0;
        token.type = TokNumber;
        token.end = m_ptr;
        const UChar* digit = token.start;
        int negative = 1;
        if (*digit == '-') {
            negative = -1;
            digit++;
        }
        
        while (digit < m_ptr)
            result = result * 10 + (*digit++) - '0';
        result *= negative;
        token.numberToken = result;
        return TokNumber;
    }

    //  ([eE][+-]? [0-9]+)?
    if (m_ptr < m_end && (*m_ptr == 'e' || *m_ptr == 'E')) { // [eE]
        ++m_ptr;

        // [-+]?
        if (m_ptr < m_end && (*m_ptr == '-' || *m_ptr == '+'))
            ++m_ptr;

        // [0-9]+
        if (m_ptr >= m_end || !isASCIIDigit(*m_ptr))
            return TokError;
        
        ++m_ptr;
        while (m_ptr < m_end && isASCIIDigit(*m_ptr))
            ++m_ptr;
    }
    
    token.type = TokNumber;
    token.end = m_ptr;
    Vector<char, 64> buffer(token.end - token.start + 1);
    int i;
    for (i = 0; i < token.end - token.start; i++) {
        ASSERT(static_cast<char>(token.start[i]) == token.start[i]);
        buffer[i] = static_cast<char>(token.start[i]);
    }
    buffer[i] = 0;
    char* end;
    token.numberToken = WTI::strtod(buffer.data(), &end);
    ASSERT(buffer.data() + (token.end - token.start) == end);
    return TokNumber;
}

TiValue LiteralParser::parse(ParserState initialState)
{
    ParserState state = initialState;
    MarkedArgumentBuffer objectStack;
    TiValue lastValue;
    Vector<ParserState, 16> stateStack;
    Vector<Identifier, 16> identifierStack;
    while (1) {
        switch(state) {
            startParseArray:
            case StartParseArray: {
                TiArray* array = constructEmptyArray(m_exec);
                objectStack.append(array);
                // fallthrough
            }
            doParseArrayStartExpression:
            case DoParseArrayStartExpression: {
                TokenType lastToken = m_lexer.currentToken().type;
                if (m_lexer.next() == TokRBracket) {
                    if (lastToken == TokComma)
                        return TiValue();
                    m_lexer.next();
                    lastValue = objectStack.last();
                    objectStack.removeLast();
                    break;
                }

                stateStack.append(DoParseArrayEndExpression);
                goto startParseExpression;
            }
            case DoParseArrayEndExpression: {
                 asArray(objectStack.last())->push(m_exec, lastValue);
                
                if (m_lexer.currentToken().type == TokComma)
                    goto doParseArrayStartExpression;

                if (m_lexer.currentToken().type != TokRBracket)
                    return TiValue();
                
                m_lexer.next();
                lastValue = objectStack.last();
                objectStack.removeLast();
                break;
            }
            startParseObject:
            case StartParseObject: {
                TiObject* object = constructEmptyObject(m_exec);
                objectStack.append(object);

                TokenType type = m_lexer.next();
                if (type == TokString || (m_mode != StrictJSON && type == TokIdentifier)) {
                    Lexer::LiteralParserToken identifierToken = m_lexer.currentToken();

                    // Check for colon
                    if (m_lexer.next() != TokColon)
                        return TiValue();
                    
                    m_lexer.next();
                    identifierStack.append(makeIdentifier(identifierToken.stringToken, identifierToken.stringLength));
                    stateStack.append(DoParseObjectEndExpression);
                    goto startParseExpression;
                }
                if (type != TokRBrace) 
                    return TiValue();
                m_lexer.next();
                lastValue = objectStack.last();
                objectStack.removeLast();
                break;
            }
            doParseObjectStartExpression:
            case DoParseObjectStartExpression: {
                TokenType type = m_lexer.next();
                if (type != TokString && (m_mode == StrictJSON || type != TokIdentifier))
                    return TiValue();
                Lexer::LiteralParserToken identifierToken = m_lexer.currentToken();

                // Check for colon
                if (m_lexer.next() != TokColon)
                    return TiValue();

                m_lexer.next();
                identifierStack.append(makeIdentifier(identifierToken.stringToken, identifierToken.stringLength));
                stateStack.append(DoParseObjectEndExpression);
                goto startParseExpression;
            }
            case DoParseObjectEndExpression:
            {
                asObject(objectStack.last())->putDirect(m_exec->globalData(), identifierStack.last(), lastValue);
                identifierStack.removeLast();
                if (m_lexer.currentToken().type == TokComma)
                    goto doParseObjectStartExpression;
                if (m_lexer.currentToken().type != TokRBrace)
                    return TiValue();
                m_lexer.next();
                lastValue = objectStack.last();
                objectStack.removeLast();
                break;
            }
            startParseExpression:
            case StartParseExpression: {
                switch (m_lexer.currentToken().type) {
                    case TokLBracket:
                        goto startParseArray;
                    case TokLBrace:
                        goto startParseObject;
                    case TokString: {
                        Lexer::LiteralParserToken stringToken = m_lexer.currentToken();
                        m_lexer.next();
                        lastValue = jsString(m_exec, makeIdentifier(stringToken.stringToken, stringToken.stringLength).ustring());
                        break;
                    }
                    case TokNumber: {
                        Lexer::LiteralParserToken numberToken = m_lexer.currentToken();
                        m_lexer.next();
                        lastValue = jsNumber(numberToken.numberToken);
                        break;
                    }
                    case TokNull:
                        m_lexer.next();
                        lastValue = jsNull();
                        break;

                    case TokTrue:
                        m_lexer.next();
                        lastValue = jsBoolean(true);
                        break;

                    case TokFalse:
                        m_lexer.next();
                        lastValue = jsBoolean(false);
                        break;

                    default:
                        // Error
                        return TiValue();
                }
                break;
            }
            case StartParseStatement: {
                switch (m_lexer.currentToken().type) {
                    case TokLBracket:
                    case TokNumber:
                    case TokString:
                        goto startParseExpression;

                    case TokLParen: {
                        m_lexer.next();
                        stateStack.append(StartParseStatementEndStatement);
                        goto startParseExpression;
                    }
                    default:
                        return TiValue();
                }
            }
            case StartParseStatementEndStatement: {
                ASSERT(stateStack.isEmpty());
                if (m_lexer.currentToken().type != TokRParen)
                    return TiValue();
                if (m_lexer.next() == TokEnd)
                    return lastValue;
                return TiValue();
            }
            default:
                ASSERT_NOT_REACHED();
        }
        if (stateStack.isEmpty())
            return lastValue;
        state = stateStack.last();
        stateStack.removeLast();
        continue;
    }
}

}
