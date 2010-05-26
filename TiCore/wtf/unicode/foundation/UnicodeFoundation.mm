/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */
#import "UnicodeFoundation.h"

#import "config.h" // Required to include Foundation.h
#import <CoreFoundation/CoreFoundation.h>

namespace WTI 
{
    namespace Unicode 
    {
        UChar32 foldCase(UChar32)
        {
            //FIXME -- currently can't find a use in KJS
            return 0;
        }
        
        int foldCase(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error)
        {
            //FIXME -- currently can't find a use in KJS
            return 0;
        }
        
        int toLower(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error)
        {
            CFMutableStringRef sourceRef = CFStringCreateMutable(NULL,srcLength);
            CFStringAppendCharacters(sourceRef, src, srcLength);
            CFStringLowercase(sourceRef, NULL);
            int resultLength = CFStringGetLength(sourceRef);
            CFStringGetCharacters(sourceRef, CFRangeMake(0, resultLength), result);
            CFRelease(sourceRef);
            return resultLength;
        }
        
        UChar32 toLower(UChar32 c)
        {
            CFMutableStringRef sourceRef = CFStringCreateMutable(NULL,1);
            UniChar character = (UniChar)c;
            CFStringAppendCharacters(sourceRef, &character, 1);
            CFStringLowercase(sourceRef, NULL);
            CFStringGetCharacters(sourceRef, CFRangeMake(0, 1), &character);
            CFRelease(sourceRef);
            return (UChar32)character;
        }
        
        UChar32 toUpper(UChar32 c)
        {
            CFMutableStringRef sourceRef = CFStringCreateMutable(NULL,1);
            UniChar character = (UniChar)c;
            CFStringAppendCharacters(sourceRef, &character, 1);
            CFStringUppercase(sourceRef, NULL);
            CFStringGetCharacters(sourceRef, CFRangeMake(0, 1), &character);
            CFRelease(sourceRef);
            return (UChar32)character;
        }
        
        int toUpper(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error)
        {
            CFMutableStringRef sourceRef = CFStringCreateMutable(NULL,srcLength);
            CFStringAppendCharacters(sourceRef, src, srcLength);
            CFStringUppercase(sourceRef, NULL);
            int resultLength = CFStringGetLength(sourceRef);
            CFStringGetCharacters(sourceRef, CFRangeMake(0, resultLength), result);
            CFRelease(sourceRef);
            return resultLength;
        }
        
        UChar32 toTitleCase(UChar32 c)
        {
            CFMutableStringRef sourceRef = CFStringCreateMutable(NULL,1);
            UniChar character = (UniChar)c;
            CFStringAppendCharacters(sourceRef, &character, 1);
            CFStringCapitalize(sourceRef, NULL);
            CFStringGetCharacters(sourceRef, CFRangeMake(0, 1), &character);
            CFRelease(sourceRef);
            return (UChar32)character;
        }
        
        bool isArabicChar(UChar32 c)
        {
            return c >= 0x0600 && c <= 0x06FF;
        }
        
        bool isFormatChar(UChar32 c)
        {
            return false; //FIXME -- currently can't find a use in KJS
        }
        
        bool isSeparatorSpace(UChar32 c)
        {
            CFCharacterSetRef charSet = CFCharacterSetGetPredefined(kCFCharacterSetWhitespace);
            return CFCharacterSetIsCharacterMember(charSet, (UniChar)c);
        }
        
        bool isPrintableChar(UChar32 c)
        {
            return false; //FIXME -- currently can't find a use in KJS
        }
        
        bool isDigit(UChar32 c)
        {
            CFCharacterSetRef charSet = CFCharacterSetGetPredefined(kCFCharacterSetDecimalDigit);
            return CFCharacterSetIsCharacterMember(charSet, (UniChar)c);
        }
        
        bool isPunct(UChar32 c)
        {
            CFCharacterSetRef charSet = CFCharacterSetGetPredefined(kCFCharacterSetPunctuation);
            return CFCharacterSetIsCharacterMember(charSet, (UniChar)c);
        }
        
        bool hasLineBreakingPropertyComplexContext(UChar32 c)
        {
            // FIXME -- currently can't find a use in KJS
            return false; 
        }
        
        bool hasLineBreakingPropertyComplexContextOrIdeographic(UChar32 c)
        {
            // FIXME -- currently can't find a use in KJS
            return false;
        }
        
        UChar32 mirroredChar(UChar32 c)
        {
            return '0'; //FIXME -- currently can't find a use in KJS
        }
        
        CharCategory category(UChar32 c)
        {
            if (c > 0xffff)
                return NoCategory;
            
            uint category = 0;
            
            if (isLower(c))
            {
                category|=Letter_Lowercase;
            }
            else
            {
                category|=Letter_Uppercase;
            }
            if (isDigit(c))
            {
                category|=Number_DecimalDigit;
            }
            if (isSeparatorSpace(c))
            {
                category|=Mark_SpacingCombining;
            }
            else 
            {
                category|=Mark_NonSpacing;
            }
            /*
            return category(c) & (Letter_Uppercase | Letter_Lowercase | Letter_Titlecase | Letter_Modifier | Letter_Other
                                  | Mark_NonSpacing | Mark_SpacingCombining | Number_DecimalDigit | Punctuation_Connector);
             */
            return (CharCategory)category;
        }
        
        Direction direction(UChar32)
        {
            //FIXME -- currently can't find a use in KJS
            return LeftToRight;
        }
        
        bool isLower(UChar32 c)
        {
            CFCharacterSetRef charSet = CFCharacterSetGetPredefined(kCFCharacterSetLowercaseLetter);
            return CFCharacterSetIsCharacterMember(charSet, (UniChar)c);
        }
        
        int digitValue(UChar32 c)
        {
            //FIXME -- currently can't find a use in KJS
            return 0;
        }
        
        uint8_t combiningClass(UChar32 c)
        {
            //FIXME -- currently can't find a use in KJS
            return 0;
        }
        
        DecompositionType decompositionType(UChar32 c)
        {
            //FIXME -- currently can't find a use in KJS
            return DecompositionNone;
        }
        
        int umemcasecmp(const UChar*, const UChar*, int len)
        {
            return 0;  //FIXME -- currently can't find a use in KJS
        }
    }
}
