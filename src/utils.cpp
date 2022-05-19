#include "pch.h"

namespace Utils
{
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

std::size_t replace_all(std::wstring& inout, std::wstring_view what, std::wstring_view with)
{
    std::size_t count{};
    for (std::wstring::size_type pos{};
        inout.npos != (pos = inout.find(what.data(), pos, what.length()));
        pos += with.length(), ++count)
    {
        inout.replace(pos, what.length(), with.data(), with.length());
    }
    return count;
}

}