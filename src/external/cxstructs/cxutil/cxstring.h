// Copyright (c) 2023 gk646
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#define CX_FINISHED
#ifndef CXSTRUCTS_SRC_CXUTIL_CXSTRING_H_
#define CXSTRUCTS_SRC_CXUTIL_CXSTRING_H_

#include <cstring>
#include <cctype>

namespace cxstructs
{
    // Measure the length of the given string "arg" using a while loop
    constexpr int str_len(const char* arg)
    {
        int len = 0;
        while (*arg != 0)
        {
            arg++;
            len++;
        }
        return len;
    }
    // Returns true if either string is a common prefix of the other - or the shorter string is a prefix of the longer
    inline bool str_cmp_prefix(const char* s1, const char* s2)
    {
        while (*s1 && *s2)
        {
            if (*s1 != *s2)
            {
                return false;
            }
            ++s1;
            ++s2;
        }
        return true;
    }
    // Returns an allocated copy of the given string "arg" (with new[]);
    [[nodiscard("Allocates new string")]] inline char* str_dup(const char* arg)
    {
        const int len = str_len(arg);
        auto* newBuff = new char[len + 1];
        for (int i = 0; i <= len; i++)
        {
            newBuff[i] = arg[i];
        }
        return newBuff;
    }
    // Compares to string with a while loop
    inline bool str_cmp(const char* arg, const char* arg2)
    {
        while (*arg != 0 && *arg2 != 0)
        {
            if (*arg != *arg2)
                return false;
            arg++;
            arg2++;
        }
        return *arg == *arg2; // Both should be '\0' if strings are truly equal
    }
    // Case insensitive! - Compares to string with a while loop - stops at max count
    inline int str_cmpn_case(const char* s1, const char* s2, int maxCount)
    {
        while (*s1 && *s2 && maxCount > 0)
        {
            const int diff = tolower(*s1) - tolower(*s2);
            if (diff != 0)
                return diff;
            ++s1;
            ++s2;
            maxCount--;
        }
        return tolower(*s1) - tolower(*s2);
    }
    // Compares the given strings from behind until either string ends or a mismatch occurs
    inline int str_cmp_rev(const char* str1, const char* str2)
    {
        if (str1 == nullptr || str2 == nullptr)
        {
            return (str1 == nullptr) - (str2 == nullptr);
        }

        const char* ptr1 = str1;
        const char* ptr2 = str2;

        // Move the pointers to the end of the strings
        while (*ptr1)
        {
            ptr1++;
        }
        while (*ptr2)
        {
            ptr2++;
        }

        // Compare from the end
        while (ptr1 != str1 && ptr2 != str2)
        {
            ptr1--;
            ptr2--;

            if (*ptr1 != *ptr2)
            {
                return *ptr1 - *ptr2;
            }
        }

        return static_cast<int>((ptr1 - str1) - (ptr2 - str2));
    }
    // Case insensitive! - Tries to find and return the first occurrence of sequence in string
    inline const char* str_substr_case(const char* string, const char* sequence)
    {
        if (!*sequence)
            return string;

        for (const char* p = string; *p; ++p)
        {
            if (tolower(*p) == tolower(*sequence))
            {
                const char *h = p, *n = sequence;
                while (*h && *n && tolower(*h) == tolower(*n))
                {
                    ++h;
                    ++n;
                }
                if (!*n)
                    return p;
            }
        }
        return nullptr;
    }
    // Parses the given string into a int on best effort basis - stops when encountering any non numeric characters
    inline int str_parse_int(const char* str, const int radix = 10)
    {
        if (str == nullptr || *str == '\0') [[unlikely]]
            return 0;

        int result = 0;
        bool negative = false;
        if (*str == '-')
        {
            negative = true;
            ++str;
        }

        while (*str)
        {
            const char digit = *str;
            int value;
            if (digit >= '0' && digit <= '9')
                value = digit - '0';
            else if (digit >= 'a' && digit <= 'z')
                value = 10 + digit - 'a';
            else if (digit >= 'A' && digit <= 'Z')
                value = 10 + digit - 'A';
            else
                break;

            if (value >= radix)
                break;

            result = result * radix + value;
            ++str;
        }

        return negative ? -result : result;
    }
    // Parses the given string into a int64 on best effort basis
    inline long long str_parse_long(const char* str, const int radix = 10)
    {
        if (str == nullptr || *str == '\0')
            return 0;

        long long result = 0;
        bool negative = false;
        if (*str == '-')
        {
            negative = true;
            ++str;
        }

        while (*str)
        {
            char digit = *str;
            long long value;
            if (digit >= '0' && digit <= '9')
                value = digit - '0';
            else if (digit >= 'a' && digit <= 'z')
                value = 10 + digit - 'a';
            else if (digit >= 'A' && digit <= 'Z')
                value = 10 + digit - 'A';
            else
                break;

            if (value >= radix)
                break;

            result = result * radix + value;
            ++str;
        }

        return negative ? -result : result;
    }
    // Parses the given string into a float on best effort basis
    inline float str_parse_float(const char* str)
    {
        if (str == nullptr || *str == '\0')
            return 0.0F;

        float result = 0.0;
        bool negative = false;
        if (*str == '-')
        {
            negative = true;
            ++str;
        }

        // Parse the integer part
        while (*str != 0 && *str != '.')
        {
            if (*str < '0' || *str > '9')
                break;
            result = result * 10 + (*str - '0');
            ++str;
        }

        // Parse the fractional part
        if (*str == '.')
        {
            ++str;
            float factor = 0.1F;
            while (*str != 0 && *str >= '0' && *str <= '9')
            {
                result += (*str - '0') * factor;
                factor *= 0.1F;
                ++str;
            }
        }

        return negative ? -result : result;
    }
    // Parses the given string into a double on best effort basis
    inline double str_parse_double(const char* str)
    {
        if (str == nullptr || *str == '\0')
            return 0.0;

        double result = 0.0;
        bool negative = false;
        if (*str == '-')
        {
            negative = true;
            ++str;
        }

        // Parse the integer part
        while (*str && *str != '.')
        {
            if (*str < '0' || *str > '9')
                break;
            result = result * 10 + (*str - '0');
            ++str;
        }

        // Parse the fractional part
        if (*str == '.')
        {
            ++str;
            double factor = 0.1;
            while (*str && *str >= '0' && *str <= '9')
            {
                result += (*str - '0') * factor;
                factor *= 0.1;
                ++str;
            }
        }

        return negative ? -result : result;
    }
    // Returns the counted characters until the given 'stop' char is encountered or 'maxCount' amount of characters are read
    inline int str_count_chars_until(const char* data, char stop, int maxCount)
    {
        int count = 0;
        while (count < maxCount && data[count] != '\0')
        {
            if (data[count] == stop)
            {
                break;
            }
            count++;
        }
        return count;
    }
    // Reads (copies) all chars from the given data into the given 'buffer' until its either full or the stop char is read
    inline void str_read_into_until(const char* data, char* buffer, size_t buffer_size, char stop)
    {
        size_t count = 0;
        while (count < buffer_size - 1 && data[count] != stop && data[count] != '\0')
        {
            buffer[count] = data[count];
            count++;
        }
        buffer[count] = '\0';
    }
    // Advances the given string pointer until the given amount of the given character is found
    inline void str_skip_char(char*& ptr, const char c, int count = 1) noexcept
    {
        while (*ptr != '\0' && count > 0)
        {
            if (*ptr == c)
            {
                --count;
            }
            ++ptr;
        }
    }
    // Returns the Levenshtein distance for the two given strings
    // This penalizes differences in length
    // Failure: returns -1
    template <int MAX_LEN>
    int str_sort_levenshtein_case(const char* s1, const char* s2, const bool caseSensitive = false)
    {
        size_t len1 = strlen(s1);
        size_t len2 = strlen(s2);

        // Soft cap: Adjust lengths to be no more than MAX_LEN
        if (static_cast<size_t>(MAX_LEN) < len1)
        {
            len1 = MAX_LEN;
        }
        if (static_cast<size_t>(MAX_LEN) < len2)
        {
            len2 = MAX_LEN;
        }

        unsigned int d[MAX_LEN + 1][MAX_LEN + 1];

        for (unsigned int i = 0; i <= len1; ++i)
            d[i][0] = i;
        for (unsigned int j = 0; j <= len2; ++j)
            d[0][j] = j;

        for (unsigned int i = 1; i <= len1; ++i)
        {
            for (unsigned int j = 1; j <= len2; ++j)
            {
                unsigned int cost;
                if (caseSensitive)
                {
                    cost = s1[i - 1] == s2[j - 1] ? 0 : 1;
                }
                else
                {
                    cost = tolower(s1[i - 1]) == tolower(s2[j - 1]) ? 0 : 1;
                }

                unsigned int above = d[i - 1][j] + 1;
                unsigned int left = d[i][j - 1] + 1;
                unsigned int diag = d[i - 1][j - 1] + cost;

                if (above <= left && above <= diag)
                {
                    d[i][j] = above;
                }
                else if (left <= above && left <= diag)
                {
                    d[i][j] = left;
                }
                else
                {
                    d[i][j] = diag;
                }
            }
        }

        return d[len1][len2];
    }
    // string hash function
    constexpr unsigned int str_hash_fnv1a_32(char const* s) noexcept
    {
        unsigned int hash = 2166136261U;
        while (*s != 0)
        {
            hash ^= (unsigned int)*s++;
            hash *= 16777619U;
        }
        return hash;
    }

    // Hash operator for a const char* for e.g. std::unordered_map
    struct Fnv1aHash
    {
        auto operator()(const char* s) const noexcept -> std::size_t
        {
            unsigned int hash = 2166136261U;
            while (*s != 0)
            {
                hash ^= (unsigned int)(*s++);
                hash *= 16777619U;
            }
            return hash;
        }
    };
    // Equal operator for a const char* for e.g. std::unordered_map
    struct StrEqual
    {
        bool operator()(const char* s1, const char* s2) const { return std::strcmp(s1, s2) == 0; }
    };
#if defined(_STRING_) || defined(_GLIBCXX_STRING) || defined(_LIBCPP_STRING) || defined(_FREEBSD_STRING_)
    // Replaces the string with the string representation of the given number
    inline void str_embed_num(std::string& s, float num)
    {
        s.clear();
        char buff[10];
        snprintf(buff, sizeof(buff), "%.6f", num);
        s.append(buff);
    }

    // Useful to have a hashmap with string key and const char* lookups without conversion
    struct StringCharHash
    {
        using is_transparent = void;
        std::size_t operator()(const std::string& key) const { return str_hash_fnv1a_32(key.c_str()); }
        std::size_t operator()(const char* key) const { return str_hash_fnv1a_32(key); }
    };

    struct StringCharEquals
    {
        using is_transparent = void;
        bool operator()(const std::string& lhs, const std::string& rhs) const
        {
            return strcmp(lhs.c_str(), rhs.c_str()) == 0;
        }
        bool operator()(const std::string& lhs, const char* rhs) const { return strcmp(lhs.c_str(), rhs) == 0; }
        bool operator()(const char* lhs, const std::string& rhs) const { return strcmp(lhs, rhs.c_str()) == 0; }
    };
#endif
} // namespace cxstructs

#endif //CXSTRUCTS_SRC_CXUTIL_CXSTRING_H_