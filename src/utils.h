#pragma once

#include "rid.h"

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

size_t UTF8_len(const string& input);

bool is_extension_allowed(const string& ext);

bool is_domain_known(const string& domain);

string env(string_cref name);

void resize_string(string& str, size_t new_len);

string get_after_from_file(const string& from);

void save_after_to_file(const string& where,
                        const string& content);

string extract_image_id_from_url(const string& url);

std::ostream& operator<<(std::ostream& os, const Download_Result& dr);

string to_str(const Download_Result& dr);

vector<string> split_string(string_cref line,
                          string_cref delimiter);

}
