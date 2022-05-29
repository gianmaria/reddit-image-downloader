#include "pch.h"
#include "utils.h"

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
        char c = *it;

        if (c != '.')
        {
            if (c == '/' and
                !found)
            {
                break; // no extension found
            }
            else
            {
                if (std::isalpha(c))
                {
                    buff << c;
                }
                else
                {
                    break; // extension with non alpha chars... ?
                }
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

size_t UTF8_len(const std::string& input)
{
    const char* data = input.data();
    size_t size = input.size();

    size_t len = 0;

    size_t i = 0;
    while (i < size)
    {
        ++len;

        uint8_t c = static_cast<uint8_t>(data[i]);

        if ((c >> 7) == 0b0) // 1 byte
        {
            i += 1;    
        }
        else if ((c >> 5) == 0b110) // 2 byte
        {
            i += 2;
        }
        else if ((c >> 4) == 0b1110) // 3 byte
        {
            i += 3;
        }
        else if ((c >> 3) == 0b11110) // 4 byte
        {
            i += 4;
        }
        else
        {
            int whut = 0;
            assert(false);
        }
    }

    return len;
}

bool is_extension_allowed(const string& ext)
{
    const vector<string> allowed_ext
    {
        "gif", "jpeg", "png",
        "jpg", "bmp"
    };

    bool res = std::any_of(
        allowed_ext.begin(),
        allowed_ext.end(),
        [&ext](const string& allowed_ext)
    {
        return allowed_ext == ext;
    });

    return res;
}

bool is_domain_known(const string& domain)
{
    const vector<string> allowed_domains
    {
        "imgur.com"
    };

    bool res = std::any_of(
        allowed_domains.begin(),
        allowed_domains.end(),
        [&domain](const string& allowed_domain)
    {
        return allowed_domain == domain;
    });

    return res;
}

string env(string_cref name)
{
    auto split_line = [](string_cref line)
        -> std::pair<string, string>
    {
        const string delimiter = "=";
        auto pos = line.find(delimiter);

        if (pos == std::string::npos)
            return {};

        std::pair<string, string> res;

        res.first = line.substr(0, pos);
        res.second = line.substr(pos + 1);

        return res;
    };

    std::ifstream ifs(".env");

    string res{};

    if (ifs.is_open())
    {
        for (string line;
             std::getline(ifs, line);
             )
        {
            auto [key, value] = split_line(line);

            if (key == name)
            {
                res = value;
                break;
            }
        }
    }

    return res;
}

void resize_string(string& str, size_t new_len)
{
    // TODO: proper resize!!!
    str.resize(new_len);
}

}