#include "pch.h"

namespace Utils
{
#if 0

std::string ConvertWideToUtf8(const std::wstring& str)
{
    LPCWCH data_ = reinterpret_cast<LPCWCH>(str.data());

    SetLastError(0);

    BOOL UsedDefaultChar;
    int count = WideCharToMultiByte(
        CP_UTF8, 0,
        data_, str.size(),
        NULL, 0,
        NULL, &UsedDefaultChar);

    std::string res(count, 0);

    int count2 = WideCharToMultiByte(
        CP_UTF8, 0,
        data_, str.size(),
        &res[0], count,
        NULL, &UsedDefaultChar);

    auto last_error = GetLastError();

    return res;
}

std::wstring ConvertUtf8ToWide(const char8_t* data, size_t size)
{
    LPCCH data_ = reinterpret_cast<LPCCH>(data);

    int count = MultiByteToWideChar(CP_UTF8, 0,
                                    data_, size, NULL, 0);

    std::wstring wstr(count, 0);

    int res = MultiByteToWideChar(CP_UTF8, 0,
                                  data_, size, &wstr[0], count);

    return wstr;
}

std::wstring ConvertUtf8ToWide(const std::string& str)
{
    return ConvertUtf8ToWide(
        reinterpret_cast<const char8_t*>(str.data()),
        str.size());
}

std::wstring ConvertUtf8ToWide(const std::u8string& str)
{
    return ConvertUtf8ToWide(
        str.data(),
        str.size());
}

std::wstring ConvertUtf8ToWide(const char* str)
{
    return ConvertUtf8ToWide(
        reinterpret_cast<const char8_t*>(str),
        static_cast<size_t>(strlen(str)));
}

#endif // 0

std::size_t replace_all(std::string& inout, std::string_view what, std::string_view with)
{
    std::size_t count{};
    for (std::string::size_type pos{};
        inout.npos != (pos = inout.find(what.data(), pos, what.length()));
        pos += with.length(), ++count)
    {
        inout.replace(pos, what.length(), with.data(), with.length());
    }
    return count;
}

std::string remove_invalid_charaters(std::string from)
{
    std::vector<char> invalid_chars =
    { '<', '>', ':', '/', '\\', '|', '?', '*', '"' };

    auto replace_fn = [&ic = std::as_const(invalid_chars)](char c1)
    {
        auto any_fn = [&c1](char c2) {return c1 == c2; };

        return std::any_of(
            ic.begin(), ic.end(),
            any_fn);
    };

    std::replace_if(
        from.begin(), from.end(),
        replace_fn, '_');

    return from;
}

std::string get_file_extension_from_url(const std::string& from)
{
    std::stringstream buff;

    bool found = false;

    for (auto it = from.rbegin();
        it != from.rend();
        ++it)
    {
        if (*it != '.')
        {
            if (*it == '/' and
                !found)
            {
                break; // no extension found
            }
            else
            {
                buff << *it;
            }
        }
        else
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        return "";
    }

    std::string ext = buff.str();
    std::reverse(ext.begin(), ext.end());

    return ext;
}

}