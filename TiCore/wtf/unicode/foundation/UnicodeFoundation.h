/**
 * Appcelerator Titanium License
 * This source code and all modifications done by Appcelerator
 * are licensed under the Apache Public License (version 2) and
 * are Copyright (c) 2009 by Appcelerator, Inc.
 */

#ifndef UnicodeFoundation_h
#define UnicodeFoundation_h

#include <stdint.h>

//rls the following macros were taken from utf16.h in icu
/**
 * Is this code point a surrogate (U+d800..U+dfff)?
 * @param c 32-bit code point
 * @return TRUE or FALSE
 * @stable ICU 2.4
 */
#define U_IS_SURROGATE(c) (((c)&0xfffff800)==0xd800)

/**
 * Does this code unit alone encode a code point (BMP, not a surrogate)?
 * @param c 16-bit code unit
 * @return TRUE or FALSE
 * @stable ICU 2.4
 */
#define U16_IS_SINGLE(c) !U_IS_SURROGATE(c)

/**
 * Is this code unit a lead surrogate (U+d800..U+dbff)?
 * @param c 16-bit code unit
 * @return TRUE or FALSE
 * @stable ICU 2.4
 */
#define U16_IS_LEAD(c) (((c)&0xfffffc00)==0xd800)

/**
 * Is this code unit a trail surrogate (U+dc00..U+dfff)?
 * @param c 16-bit code unit
 * @return TRUE or FALSE
 * @stable ICU 2.4
 */
#define U16_IS_TRAIL(c) (((c)&0xfffffc00)==0xdc00)

/**
 * Helper constant for U16_GET_SUPPLEMENTARY.
 * @internal
 */
#define U16_SURROGATE_OFFSET ((0xd800<<10UL)+0xdc00-0x10000)

/**
 * Get a supplementary code point value (U+10000..U+10ffff)
 * from its lead and trail surrogates.
 * The result is undefined if the input values are not
 * lead and trail surrogates.
 *
 * @param lead lead surrogate (U+d800..U+dbff)
 * @param trail trail surrogate (U+dc00..U+dfff)
 * @return supplementary code point (U+10000..U+10ffff)
 * @stable ICU 2.4
 */
#define U16_GET_SUPPLEMENTARY(lead, trail) \
(((UChar32)(lead)<<10UL)+(UChar32)(trail)-U16_SURROGATE_OFFSET)

typedef uint16_t UChar;
typedef int32_t UChar32;

namespace WTI {
    namespace Unicode {
        
        enum Direction {
            LeftToRight,
            RightToLeft,
            EuropeanNumber,
            EuropeanNumberSeparator,
            EuropeanNumberTerminator,
            ArabicNumber,
            CommonNumberSeparator,
            BlockSeparator,
            SegmentSeparator,
            WhiteSpaceNeutral,
            OtherNeutral,
            LeftToRightEmbedding,
            LeftToRightOverride,
            RightToLeftArabic,
            RightToLeftEmbedding,
            RightToLeftOverride,
            PopDirectionalFormat,
            NonSpacingMark,
            BoundaryNeutral
        };
        
        enum DecompositionType {
            DecompositionNone,
            DecompositionCanonical,
            DecompositionCompat,
            DecompositionCircle,
            DecompositionFinal,
            DecompositionFont,
            DecompositionFraction,
            DecompositionInitial,
            DecompositionIsolated,
            DecompositionMedial,
            DecompositionNarrow,
            DecompositionNoBreak,
            DecompositionSmall,
            DecompositionSquare,
            DecompositionSub,
            DecompositionSuper,
            DecompositionVertical,
            DecompositionWide,
        };
        
        enum CharCategory {
            NoCategory =  0,
            Other_NotAssigned,
            Letter_Uppercase,
            Letter_Lowercase,
            Letter_Titlecase,
            Letter_Modifier,
            Letter_Other,
            
            Mark_NonSpacing,
            Mark_Enclosing,
            Mark_SpacingCombining,
            
            Number_DecimalDigit,
            Number_Letter,
            Number_Other,
            
            Separator_Space,
            Separator_Line,
            Separator_Paragraph,
            
            Other_Control,
            Other_Format,
            Other_PrivateUse,
            Other_Surrogate,
            
            Punctuation_Dash,
            Punctuation_Open,
            Punctuation_Close,
            Punctuation_Connector,
            Punctuation_Other,
            
            Symbol_Math,
            Symbol_Currency,
            Symbol_Modifier,
            Symbol_Other,
            
            Punctuation_InitialQuote,
            Punctuation_FinalQuote
        };
        
        UChar32 foldCase(UChar32);

        int foldCase(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error);
        
        int toLower(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error);
        
        UChar32 toLower(UChar32 c);
        
        UChar32 toUpper(UChar32 c);
        
        int toUpper(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error);
        
        UChar32 toTitleCase(UChar32 c);
        
        bool isArabicChar(UChar32 c);
        
        bool isFormatChar(UChar32 c);
        
        bool isSeparatorSpace(UChar32 c);
        
        bool isPrintableChar(UChar32 c);
        
        bool isDigit(UChar32 c);
        
        bool isPunct(UChar32 c);
        
        bool hasLineBreakingPropertyComplexContext(UChar32 c);
        
        bool hasLineBreakingPropertyComplexContextOrIdeographic(UChar32 c);
        
        UChar32 mirroredChar(UChar32 c);
        
        CharCategory category(UChar32 c);
        
        Direction direction(UChar32);

        bool isLower(UChar32 c);
        
        int digitValue(UChar32 c);
        
        uint8_t combiningClass(UChar32 c);
        
        DecompositionType decompositionType(UChar32 c);
        
        int umemcasecmp(const UChar*, const UChar*, int len);
        
    }
}

#endif

