#pragma once

namespace Utils
{

std::string ConvertWideToUtf8(const std::wstring& str);

std::wstring ConvertUtf8ToWide(const char8_t* data, size_t size);

std::wstring ConvertUtf8ToWide(const std::string& str);

std::wstring ConvertUtf8ToWide(const std::u8string& str);

std::wstring ConvertUtf8ToWide(const char* str);

std::size_t replace_all(std::wstring& inout, std::wstring_view what, std::wstring_view with);

}
