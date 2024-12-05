
#include "text.h"
#include "mathematics.h"



// Utf conversions adapted from <https://github.com/Davipb/utf8-utf16-converter>



//- Preprocessor definitions

//~ UTF Conversion

#define UTF16_MAX   0xFFFF
#define UTF_MAX     0x10FFFF
#define UTF_INVALID 0xFFFD

#define UTF_GEN_SURROGATE_VALUE 0xD800
#define UTF_GEN_SURROGATE_MASK  0xF800

#define UTF_HIGH_SURROGATE_VALUE    0xD800
#define UTF_LOW_SURROGATE_VALUE     0xDC00
#define UTF_HIGH_LOW_SURROGATE_MASK 0xFC00

#define UTF_SURROGATE_CODEPOINT_OFFSET 0x10000
#define UTF_SURROGATE_CODEPOINT_MASK   0x03ff
#define UTF_SURROGATE_CODEPOINT_BITS   10

#define UTF8_1B_MAX 0x7F
#define UTF8_2B_MAX 0x7FF
#define UTF8_3B_MAX 0xFFFF
#define UTF8_4B_MAX 0x10FFFF

#define UTF8_CONTINUATION_VALUE          0x80
#define UTF8_CONTINUATION_MASK           0XC0
#define UTF8_CONTINUATION_CODEPOINT_BITS 6



//~ Local type definitions

struct UTF8Pattern
{
    u8 mask;
    u8 value;
};

#define LEADING_BYTES_LENGTH 4
static const UTF8Pattern leading_bytes[LEADING_BYTES_LENGTH] = 
{
    { 0x80, 0x00 }, // 0xxxxxxx
    { 0xE0, 0xC0 }, // 110xxxxx
    { 0xF0, 0xE0 }, // 1110xxxx
    { 0xF8, 0xF0 }  // 11110xxx
};

typedef u32 codepoint;



//- Static helper functions

static inline codepoint UTF16Decode(const c16* utf16, u64 length, u64* index)
{
    c16 high = utf16[*index];
    
    // BMP character
    if ((high & UTF_GEN_SURROGATE_MASK) != UTF_GEN_SURROGATE_VALUE)
        return high; 
    
    // Unmatched low surrogate, invalid
    if ((high & UTF_HIGH_LOW_SURROGATE_MASK) != UTF_HIGH_SURROGATE_VALUE)
        return UTF_INVALID;
    
    // String ended with an unmatched high surrogate, invalid
    if (*index == length - 1)
        return UTF_INVALID;
    
    c16 low = utf16[*index + 1];
    
    // Unmatched high surrogate, invalid
    if ((low & UTF_HIGH_LOW_SURROGATE_MASK) != UTF_LOW_SURROGATE_VALUE)
        return UTF_INVALID;
    
    // Two correctly matched surrogates, increase index to indicate we've consumed
    // two characters
    (*index)++;
    
    // The high bits of the codepoint are the value bits of the high surrogate
    // The low bits of the codepoint are the value bits of the low surrogate
    codepoint result = high & UTF_SURROGATE_CODEPOINT_MASK;
    result <<= UTF_SURROGATE_CODEPOINT_BITS;
    result |= low & UTF_SURROGATE_CODEPOINT_MASK;
    result += UTF_SURROGATE_CODEPOINT_OFFSET;
    
    // And if all else fails, it's valid
    return result;
}

static inline i32 UTF8Length(codepoint c)
{
    // An array with the max values would be more elegant, but a bit too heavy
    // for this common function
    
    if (c <= UTF8_1B_MAX)
        return 1;
    
    if (c <= UTF8_2B_MAX)
        return 2;
    
    if (c <= UTF8_3B_MAX)
        return 3;
    
    return 4;
}

static inline u64 UTF8Encode(codepoint c, c8* utf8, u64 len, u64 index)
{
    i32 size = UTF8Length(c);
    
    // Not enough space left on the string
    if (index + size > len)
        return 0;
    
    // Write the continuation bytes in reverse order first
    for (i32 i = size - 1; i > 0; i--)
    {
        u8 cont = (u8)c & ~UTF8_CONTINUATION_MASK;
        cont |= UTF8_CONTINUATION_VALUE;
        
        ((u8*)(utf8))[index + i] = cont;
        c >>= UTF8_CONTINUATION_CODEPOINT_BITS;
    }
    
    // Write the leading byte
    UTF8Pattern pattern = leading_bytes[size - 1];
    
    u8 lead = c & ~(pattern.mask);
    lead |= pattern.value;
    
    
    ((u8*)(utf8))[index] = lead;
    
    return size;
}

u64 UTF16ToUTF8(const c16* utf16, u64 utf16_len, c8* utf8, u64 utf8_len)
{
    // The next codepoint that will be written in the UTF-8 string
    // or the size of the required buffer if utf8 is NULL
    u64 utf8_index = 0;
    
    for (u64 utf16_index = 0; utf16_index < utf16_len; utf16_index++)
    {
        codepoint c = UTF16Decode(utf16, utf16_len, &utf16_index);
        
        if (utf8 == nullptr)
            utf8_index += UTF8Length(c);
        else
            utf8_index += UTF8Encode(c, utf8, utf8_len, utf8_index);
    }
    
    return utf8_index;
}

static codepoint UTF8Decode(const c8 * utf8, u64 len, u64* index)
{
    u8 leading = ((const u8*)utf8)[*index];
    
    // The number of bytes that are used to encode the codepoint
    i32 encoding_len = 0;
    // The pattern of the leading byte
    UTF8Pattern leading_pattern;
    // If the leading byte matches the current leading pattern
    b8 matches = false;
    
    do
    {
        encoding_len++;
        leading_pattern = leading_bytes[encoding_len - 1];
        
        matches = (leading & leading_pattern.mask) == leading_pattern.value;
        
    } 
    while (!matches && encoding_len < LEADING_BYTES_LENGTH);
    
    // Leading byte doesn't match any known pattern, consider it invalid
    if (!matches)
        return UTF_INVALID;
    
    codepoint c = leading & ~leading_pattern.mask;
    
    for (i32 i = 0; i < encoding_len - 1; i++)
    {
        // String ended before all continuation bytes were found
        // Invalid encoding
        if (*index + 1 >= len)
            return UTF_INVALID;
        
        u8 continuation = ((const u8*)utf8)[*index + 1];
        
        // Number of continuation bytes not the same as advertised on the leading byte
        // Invalid encoding
        if ((continuation & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_VALUE)
            return UTF_INVALID;
        
        c <<= UTF8_CONTINUATION_CODEPOINT_BITS;
        c |= continuation & ~UTF8_CONTINUATION_MASK;
        
        (*index)++;
    }
    
    i32 proper_len = UTF8Length(c);
    
    // Overlong encoding: too many bytes were used to encode a short codepoint
    // Invalid encoding
    if (proper_len != encoding_len)
        return UTF_INVALID;
    
    // Surrogates are invalid Unicode codepoints, and should only be used in UTF-16
    // Invalid encoding
    if (c < UTF16_MAX && (c & UTF_GEN_SURROGATE_MASK) == UTF_GEN_SURROGATE_VALUE)
        return UTF_INVALID;
    
    // UTF-8 can encode codepoints larger than the Unicode standard allows
    // Invalid encoding
    if (c > UTF_MAX)
        return UTF_INVALID;
    
    return c;
}

static inline i32 UTF16Length(codepoint c)
{
    if (c <= UTF16_MAX)
        return 1;
    
    return 2;
}

static u64 UTF16Encode(codepoint c, c16* utf16, u64 len, u64 index)
{
    // Not enough space on the string
    if (index >= len)
        return 0;
    
    if (c <= UTF16_MAX)
    {
        utf16[index] = c;
        return 1;
    }
    
    // Not enough space on the string for two surrogates
    if (index + 1 >= len)
        return 0;
    
    c -= UTF_SURROGATE_CODEPOINT_OFFSET;
    
    c16 low = UTF_LOW_SURROGATE_VALUE;
    low |= c & UTF_SURROGATE_CODEPOINT_MASK;
    
    c >>= UTF_SURROGATE_CODEPOINT_BITS;
    
    c16 high = UTF_HIGH_SURROGATE_VALUE;
    high |= c & UTF_SURROGATE_CODEPOINT_MASK;
    
    utf16[index] = high;
    utf16[index + 1] = low;
    
    return 2;
}

static inline u64 UTF8ToUTF16(const c8* utf8, u64 utf8_len, c16* utf16, u64 utf16_len)
{
    // The next codepoint that will be written in the UTF-16 string
    // or the size of the required buffer if utf16 is NULL
    u64 utf16_index = 0;
    
    for (u64 utf8_index = 0; utf8_index < utf8_len; utf8_index++)
    {
        codepoint c = UTF8Decode(utf8, utf8_len, &utf8_index);
        
        if (utf16 == nullptr)
            utf16_index += UTF16Length(c);
        else
            utf16_index += UTF16Encode(c, utf16, utf16_len, utf16_index);
    }
    
    return utf16_index;
}





namespace Tool
{
    //~ Working variables
    
    static u8* intBuffer = nullptr;
    u64 intBufferSize = 0;
    
    static inline void SizeBuffer(u64 size)
    {
        if (intBufferSize >= size + 2)
        {
            return;
        }
        
        u64 newSize = intBufferSize;
        
        if (newSize == 0)
        {
            newSize = 256;
        }
        
        while (newSize < size + 2)
        {
            newSize *= 2;
        }
        
        if (intBuffer == nullptr)
        {
            intBuffer = (u8*)ClassicAlloc(newSize);
        }
        else
        {
            ClassicRealloc((void**)&intBuffer, intBufferSize, newSize);
        }
        
        intBufferSize = newSize;
    }
    
    
    
    //- Function definitions
    
    //~ C-string
    
    u64 CStr8Size(const c8* string, u64 capacity)
    {
        u64 size = 0;
        for (u64 i = 0; i * sizeof(c8) < capacity - 1; i++)
        {
            size = i;
            if (string[i] == '\0')
            {
                break;
            }
        }
        
        return size;
    }
    
    u64 CStr8Copy(c8* destination, const c8* source, u64 capacity, b8 terminate)
    {
        u64 size = CStr8Size(source);
        Copy((void*)destination, (const void*)source, TOOL_MIN(size + terminate * 1, capacity - terminate * 1));
        return size;
    }
    
    u64 CStr16Size(const c16* string, u64 capacity)
    {
        return CStr16Count(string, capacity) * sizeof(c16);
    }
    
    u64 CStr16Count(const c16* string, u64 capacity)
    {
        u64 count = 0;
        for (u64 i = 0; i * sizeof(c16) < capacity - 1; i++)
        {
            count = i;
            if (string[i] == u'\0')
            {
                break;
            }
        }
        
        return count;
    }
    
    u64 CStr16Copy(c16* destination, const c16* source, u64 capacity, b8 terminate)
    {
        u64 count = CStr16Count(source);
        Copy((void*)destination, (const void*)source, TOOL_MIN(count + terminate * 1, capacity - terminate * 1) * sizeof(c16));
        return count;
    }
    
    //~ UTF-8
    
    s8 S8FromS16(s16 str, MemoryAllocator a)
    {
        SizeBuffer(str.size / 2 + 1);
        
        // Do the conversion
        u64 size = UTF16ToUTF8(str.str, str.size, (c8*)intBuffer, intBufferSize);
        
        // Form data structure and transfer text memory
        s8 newString = { (c8*)AllocatorAlloc(a, size + 1, sizeof(c8)), size };
        Copy(newString.str, intBuffer, size, sizeof(c8));
        newString.str[size] = '\0';
        
        return newString;
    }
    
    s8 S8FromCStr8(const c8* cstr, MemoryAllocator a, u64 capacity)
    {
        u64 cstrSize = CStr8Size(cstr, capacity);
        
        s8 newString = { (c8*)AllocatorAlloc(a, cstrSize + 1, sizeof(c8)), cstrSize };
        Copy(newString.str, cstr, cstrSize, sizeof(c8));
        newString.str[cstrSize] = '\0';
        
        return newString;
    }
    
    s8 S8FromCStr16(const c16* cstr, MemoryAllocator a, u64 capacity)
    {
        u64 cstrCount = CStr16Count(cstr, capacity);
        
        SizeBuffer((cstrCount + 1) * sizeof(s8));
        
        // Do the conversion
        u64 size = UTF16ToUTF8(cstr, cstrCount, (c8*)intBuffer, intBufferSize);
        
        // Form data structure and transfer text memory
        s8 newString = { (c8*)AllocatorAlloc(a, size + 1, sizeof(c8)), size };
        Copy(newString.str, intBuffer, size, sizeof(c8));
        newString.str[size] = '\0';
        
        return newString;
    }
    
    //~ UTF-16
    
    s16 S16FromS8(s8 str, MemoryAllocator a)
    {
        SizeBuffer((str.size + 1) * 2);
        
        // Do the conversion
        u64 size = UTF8ToUTF16(str.str, str.size, (c16*)intBuffer, intBufferSize);
        
        // Form data structure and transfer text memory
        s16 newString = { (c16*)AllocatorAlloc(a, size + 1, sizeof(c16)), size };
        Copy(newString.str, intBuffer, size, sizeof(c16));
        newString.str[size] = u'\0';
        
        return newString;
    }
    
    s16 S16FromCStr8(const c8* cstr, MemoryAllocator a, u64 capacity)
    {
        u64 cstrSize = CStr8Size(cstr, capacity);
        
        SizeBuffer((cstrSize + 1) * 2);
        
        // Do the conversion
        u64 count = UTF8ToUTF16(cstr, cstrSize, (c16*)intBuffer, intBufferSize);
        
        // Form data structure and transfer text memory
        s16 newString = { (c16*)AllocatorAlloc(a, count + 1, sizeof(c16)), count };
        Copy(newString.str, intBuffer, count, sizeof(c16));
        newString.str[count] = u'\0';
        
        return newString;
    }
    
    s16 S16FromCStr16(const c16* cstr, MemoryAllocator a, u64 capacity)
    {
        u64 cstrCount = CStr16Count(cstr, capacity);
        
        s16 newString = { (c16*)AllocatorAlloc(a, cstrCount + 1, sizeof(c16)), cstrCount };
        Copy(newString.str, cstr, cstrCount, sizeof(c16));
        newString.str[cstrCount] = u'\0';
        
        return newString;
    }
    
    
    
    //- Other utilities
    
    //~ String Builder helper functions
    
    static void StringBuilderAddConst(StringBuilder* builder, const c8* string, u64 size)
    {
        switch (builder->type)
        {
            case StringTypeUTF8:
            {
                u64 offset = builder->size;
                builder->size += size; 
                RegionCommit(&builder->region, builder->size + 1);
                Copy(builder->str8 + offset, string, size);
                builder->str8[builder->size] = '\0';
            }
            break;
            
            case StringTypeUTF16:
            {
                u64 offset = builder->size / 2;
                u64 needed = size * 2 + 1;
                RegionCommit(&builder->region, builder->size + needed);
                u64 realCount = UTF8ToUTF16(string, size, builder->str16 + offset, needed); // TODO(crazy): Maybe this is technical debt?
                builder->size += realCount * 2; 
                builder->str16[builder->size / 2] = L'\0';
            }
            break;
            
            default:
            break;
        }
    }
    
    static void StringBuilderAddConst(StringBuilder* builder, const c16* string, u64 size)
    {
        switch (builder->type)
        {
            case StringTypeUTF8:
            {
                u64 offset = builder->size;
                u64 needed = size * 2 + 1;
                RegionCommit(&builder->region, builder->size + needed);
                u64 realSize = UTF16ToUTF8(string, size, builder->str8 + offset, needed); // TODO(crazy): Maybe this is technical debt?
                builder->size += realSize; 
                builder->str8[builder->size] = '\0';
            }
            break;
            
            case StringTypeUTF16:
            {
                u64 offset = builder->size / 2;
                u64 realSize = size * 2;
                builder->size += realSize; 
                RegionCommit(&builder->region, builder->size + 1);
                Copy(builder->str16 + offset, string, realSize);
                builder->str16[builder->size / 2] = L'\0';
                
            }
            break;
            
            default:
            break;
        }
    }
    
    //~ String builder
    
    void StringBuilderInit(StringBuilder* builder, u64 capacity, StringType type)
    {
        RegionReserve(&builder->region, capacity);
        RegionCommit(&builder->region, TOOL_MIN(capacity, 16));
        
        builder->type = type;
        builder->size = 0;
        builder->str8 = (c8*)builder->region.start;
        builder->str16[0] = L'\0';
    }
    
    void StringBuilderDestroy(StringBuilder* builder)
    {
        RegionDealloc(&builder->region);
        
        builder->type = StringTypeNone;
        builder->size = 0;
        builder->str8 = nullptr;
    }
    
    void StringBuilderReset(StringBuilder* builder)
    {
        builder->size = 0;
        builder->str16[0] = (c16)L'\0';
    }
    
    void StringBuilderAdd(StringBuilder* builder, const s8* string)
    {
        StringBuilderAddConst(builder, string->str, string->size);
    }
    
    void StringBuilderAdd(StringBuilder* builder, const s16* string)
    {
        StringBuilderAddConst(builder, string->str, string->size);
    }
    
    void StringBuilderAdd(StringBuilder* builder, const c8* cstr)
    {
        u64 size = CStr8Size(cstr);
        StringBuilderAddConst(builder, cstr, size);
    }
    
    void StringBuilderAdd(StringBuilder* builder, const c16* cstr)
    {
        u64 count = CStr16Count(cstr);
        StringBuilderAddConst(builder, cstr, count);
    }
}
