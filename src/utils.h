#pragma once

namespace Utils
{

#if 0
std::string ConvertWideToUtf8(const std::wstring& str);

std::wstring ConvertUtf8ToWide(const char8_t* data, size_t size);

std::wstring ConvertUtf8ToWide(const std::string& str);

std::wstring ConvertUtf8ToWide(const std::u8string& str);

std::wstring ConvertUtf8ToWide(const char* str);

#endif // 0


std::size_t replace_all(std::string& inout, std::string_view what, std::string_view with);

std::string remove_invalid_charaters(std::string from);

std::string get_file_extension_from_url(const std::string& from);

}
