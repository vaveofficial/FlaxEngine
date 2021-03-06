// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "Engine/Platform/StringUtils.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Collections/Array.h"
#if PLATFORM_TEXT_IS_CHAR16
#include <string>
#endif

const char DirectorySeparatorChar = '\\';
const char AltDirectorySeparatorChar = '/';
const char VolumeSeparatorChar = ':';

const Char* StringUtils::FindIgnoreCase(const Char* str, const Char* toFind)
{
    // Validate input
    if (toFind == nullptr || str == nullptr)
    {
        return nullptr;
    }

    // Get upper-case first letter of the find string (to reduce the number of full strnicmps)
    Char findInitial = ToUpper(*toFind);

    // Get length of find string, and increment past first letter
    int32 length = Length(toFind++) - 1;

    // Get the first letter of the search string, and increment past it
    Char strChar = *str++;

    // While we aren't at end of string
    while (strChar)
    {
        // Make sure it's upper-case
        strChar = ToUpper(strChar);

        // If it matches the first letter of the find string, do a case-insensitive string compare for the length of the find string
        if (strChar == findInitial && !CompareIgnoreCase(str, toFind, length))
        {
            // If we found the string, then return a pointer to the beginning of it in the search string
            return str - 1;
        }

        // Go to next letter
        strChar = *str++;
    }

    // Nothing found
    return nullptr;
}

const char* StringUtils::FindIgnoreCase(const char* str, const char* toFind)
{
    // Validate input
    if (toFind == nullptr || str == nullptr)
    {
        return nullptr;
    }

    // Get upper-case first letter of the find string (to reduce the number of full strnicmps)
    char findInitial = (char)ToUpper(*toFind);

    // Get length of find string, and increment past first letter
    int32 length = Length(toFind++) - 1;

    // Get the first letter of the search string, and increment past it
    char strChar = *str++;

    // While we aren't at end of string
    while (strChar)
    {
        // Make sure it's upper-case
        strChar = (char)ToUpper(strChar);

        // If it matches the first letter of the find string, do a case-insensitive string compare for the length of the find string
        if (strChar == findInitial && !CompareIgnoreCase(str, toFind, length))
        {
            // If we found the string, then return a pointer to the beginning of it in the search string
            return str - 1;
        }

        // Go to next letter
        strChar = *str++;
    }

    // Nothing found
    return nullptr;
}

void StringUtils::ConvertUTF82UTF16(const char* from, Char* to, uint32 fromLength, uint32* toLength)
{
    Array<unsigned long> unicode;
    uint32 i = 0;
    *toLength = 0;
    while (i < fromLength)
    {
        unsigned long uni;
        uint32 todo;
        unsigned char ch = from[i++];

        if (ch <= 0x7F)
        {
            uni = ch;
            todo = 0;
        }
        else if (ch <= 0xBF)
        {
            LOG(Error, "Not a UTF-8 string.");
            return;
        }
        else if (ch <= 0xDF)
        {
            uni = ch & 0x1F;
            todo = 1;
        }
        else if (ch <= 0xEF)
        {
            uni = ch & 0x0F;
            todo = 2;
        }
        else if (ch <= 0xF7)
        {
            uni = ch & 0x07;
            todo = 3;
        }
        else
        {
            LOG(Error, "Not a UTF-8 string.");
            return;
        }

        for (uint32 j = 0; j < todo; j++)
        {
            if (i == fromLength)
            {
                LOG(Error, "Not a UTF-8 string.");
                return;
            }
            ch = from[i++];
            if (ch < 0x80 || ch > 0xBF)
            {
                LOG(Error, "Not a UTF-8 string.");
                return;
            }

            uni <<= 6;
            uni += ch & 0x3F;
        }

        if ((uni >= 0xD800 && uni <= 0xDFFF) || uni > 0x10FFFF)
        {
            LOG(Error, "Not a UTF-8 string.");
            return;
        }

        unicode.Add(uni);
    }

    // Count chars
    uint32 length = (uint32)unicode.Count();
    for (i = 0; i < length; i++)
    {
        if (unicode[i] > 0xFFFF)
        {
            length++;
        }
    }

    // Copy chars
    *toLength = length;
    for (i = 0; i < length; i++)
    {
        unsigned long uni = unicode[i];
        if (uni <= 0xFFFF)
        {
            to[i] = (Char)uni;
        }
        else
        {
            uni -= 0x10000;
            to[i++] += (Char)((uni >> 10) + 0xD800);
            to[i] += (Char)((uni & 0x3FF) + 0xDC00);
        }
    }
}

void RemoveLongPathPrefix(const String& path, String& result)
{
    if (!path.StartsWith(TEXT("\\\\?\\"), StringSearchCase::CaseSensitive))
    {
        result = path;
    }
    if (!path.StartsWith(TEXT("\\\\?\\UNC\\"), StringSearchCase::IgnoreCase))
    {
        result = path.Substring(4);
    }
    result = path;
    result.Remove(2, 6);
}

String StringUtils::GetDirectoryName(const String& path)
{
    const int32 lastFrontSlash = path.FindLast('\\');
    const int32 lastBackSlash = path.FindLast('/');
    const int32 splitIndex = Math::Max(lastBackSlash, lastFrontSlash);
    return splitIndex != INVALID_INDEX ? path.Left(splitIndex) : String::Empty;
}

String StringUtils::GetFileName(const String& path)
{
    Char chr;
    const int32 length = path.Length();
    int32 num = length;

    do
    {
        num--;
        if (num < 0)
            return path;
        chr = path[num];
    } while (chr != DirectorySeparatorChar && chr != AltDirectorySeparatorChar && chr != VolumeSeparatorChar);

    return path.Substring(num + 1, length - num - 1);
}

String StringUtils::GetFileNameWithoutExtension(const String& path)
{
    String filename = GetFileName(path);
    const int32 num = filename.FindLast('.');
    if (num != -1)
    {
        return filename.Substring(0, num);
    }
    return filename;
}

String StringUtils::GetPathWithoutExtension(const String& path)
{
    const int32 num = path.FindLast('.');
    if (num != -1)
    {
        return path.Substring(0, num);
    }
    return path;
}

void StringUtils::PathRemoveRelativeParts(String& path)
{
    FileSystem::NormalizePath(path);

    Array<String> components;
    path.Split(TEXT('/'), components);

    Array<String> stack;
    for (auto& bit : components)
    {
        if (bit == TEXT(".."))
        {
            if (stack.HasItems())
            {
                auto popped = stack.Pop();
                if (popped == TEXT(".."))
                {
                    stack.Push(popped);
                    stack.Push(bit);
                }
            }
            else
            {
                stack.Push(bit);
            }
        }
        else if (bit == TEXT("."))
        {
            // Skip /./
        }
        else
        {
            stack.Push(bit);
        }
    }

    bool isRooted = path.StartsWith(TEXT('/'));
    path.Clear();
    for (auto& e : stack)
        path /= e;
    if (isRooted)
        path.Insert(0, TEXT("/"));
}

const char digit_pairs[201] = {
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899"
};

#define STRING_UTILS_ITOSTR_BUFFER_SIZE 15

bool StringUtils::Parse(const Char* str, float* result)
{
#if PLATFORM_TEXT_IS_CHAR16
    std::u16string u16str = str;
    std::wstring wstr(u16str.begin(), u16str.end());
    float v = wcstof(wstr.c_str(), nullptr);
#else
    float v = wcstof(str, nullptr);
#endif
    *result = v;
    if (v == 0)
    {
        const int32 len = Length(str);
        return !(str[0] == '0' && ((len == 1) || (len == 3 && (str[1] == ',' || str[1] == '.') && str[2] == '0')));
    }
    return false;
}

String StringUtils::ToString(int32 value)
{
    char buf[STRING_UTILS_ITOSTR_BUFFER_SIZE];
    char* it = &buf[STRING_UTILS_ITOSTR_BUFFER_SIZE - 2];

    int32 div = value / 100;

    if (value >= 0)
    {
        while (div)
        {
            Platform::MemoryCopy(it, &digit_pairs[2 * (value - div * 100)], 2);
            value = div;
            it -= 2;
            div = value / 100;
        }

        Platform::MemoryCopy(it, &digit_pairs[2 * value], 2);

        if (value < 10)
            it++;
    }
    else
    {
        while (div)
        {
            Platform::MemoryCopy(it, &digit_pairs[-2 * (value - div * 100)], 2);
            value = div;
            it -= 2;
            div = value / 100;
        }

        Platform::MemoryCopy(it, &digit_pairs[-2 * value], 2);

        if (value <= -10)
            it--;

        *it = '-';
    }

    return String(it, (int32)(&buf[STRING_UTILS_ITOSTR_BUFFER_SIZE] - it));
}

String StringUtils::ToString(int64 value)
{
    char buf[STRING_UTILS_ITOSTR_BUFFER_SIZE];
    char* it = &buf[STRING_UTILS_ITOSTR_BUFFER_SIZE - 2];

    int64 div = value / 100;

    if (value >= 0)
    {
        while (div)
        {
            Platform::MemoryCopy(it, &digit_pairs[2 * (value - div * 100)], 2);
            value = div;
            it -= 2;
            div = value / 100;
        }

        Platform::MemoryCopy(it, &digit_pairs[2 * value], 2);

        if (value < 10)
            it++;
    }
    else
    {
        while (div)
        {
            Platform::MemoryCopy(it, &digit_pairs[-2 * (value - div * 100)], 2);
            value = div;
            it -= 2;
            div = value / 100;
        }

        Platform::MemoryCopy(it, &digit_pairs[-2 * value], 2);

        if (value <= -10)
            it--;

        *it = '-';
    }

    return String(it, (int32)(&buf[STRING_UTILS_ITOSTR_BUFFER_SIZE] - it));
}

String StringUtils::ToString(uint32 value)
{
    char buf[STRING_UTILS_ITOSTR_BUFFER_SIZE];
    char* it = &buf[STRING_UTILS_ITOSTR_BUFFER_SIZE - 2];

    int32 div = value / 100;
    while (div)
    {
        Platform::MemoryCopy(it, &digit_pairs[2 * (value - div * 100)], 2);
        value = div;
        it -= 2;
        div = value / 100;
    }

    Platform::MemoryCopy(it, &digit_pairs[2 * value], 2);

    if (value < 10)
        it++;

    return String((char*)it, (int32)((char*)&buf[STRING_UTILS_ITOSTR_BUFFER_SIZE] - (char*)it));
}

String StringUtils::ToString(uint64 value)
{
    char buf[STRING_UTILS_ITOSTR_BUFFER_SIZE];
    char* it = &buf[STRING_UTILS_ITOSTR_BUFFER_SIZE - 2];

    int64 div = value / 100;
    while (div)
    {
        Platform::MemoryCopy(it, &digit_pairs[2 * (value - div * 100)], 2);
        value = div;
        it -= 2;
        div = value / 100;
    }

    Platform::MemoryCopy(it, &digit_pairs[2 * value], 2);

    if (value < 10)
        it++;

    return String((char*)it, (int32)((char*)&buf[STRING_UTILS_ITOSTR_BUFFER_SIZE] - (char*)it));
}

String StringUtils::ToString(float value)
{
    return String::Format(TEXT("{}"), value);
}

String StringUtils::ToString(double value)
{
    return String::Format(TEXT("{}"), value);
}

#undef STRING_UTILS_ITOSTR_BUFFER_SIZE
