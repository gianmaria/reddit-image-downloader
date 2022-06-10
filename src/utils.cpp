#include "pch.h"

#include "rid.h"

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
                if (std::isalnum(c))
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

    size_t num_of_characters = 0;

    size_t offset = 0;
    while (offset < size)
    {
        ++num_of_characters;

        uint8_t c = static_cast<uint8_t>(data[offset]);

        if ((c >> 7) == 0b0) // 1 byte
        {
            offset += 1;
        }
        else if ((c >> 5) == 0b110) // 2 byte
        {
            offset += 2;
        }
        else if ((c >> 4) == 0b1110) // 3 byte
        {
            offset += 3;
        }
        else if ((c >> 3) == 0b11110) // 4 byte
        {
            offset += 4;
        }
        else
        {
            auto emsg = std::format("Invalid character in string while calculating len, character: '{}' offset: {}",
                                    c, offset);
            throw std::runtime_error(emsg);
        }
    }

    return num_of_characters;
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

void resize_string(string& str, size_t new_len_in_characters)
{
    size_t num_of_characters = 0;

    size_t offset = 0;
    while (offset < str.size())
    {
        uint8_t c = static_cast<uint8_t>(str[offset]);

        if (num_of_characters >= new_len_in_characters)
            break;

        if ((c >> 7) == 0b0) // 1 byte
        {
            offset += 1;
        }
        else if ((c >> 5) == 0b110) // 2 byte
        {
            offset += 2;
        }
        else if ((c >> 4) == 0b1110) // 3 byte
        {
            offset += 3;
        }
        else if ((c >> 3) == 0b11110) // 4 byte
        {
            offset += 4;
        }
        else
        {
            auto emsg = std::format("Invalid value in string while resizing, value: '{}' offset: {}",
                                    c, offset);
            throw std::runtime_error(emsg);
        }

        ++num_of_characters;
    }

    str.resize(offset);
}

string get_after_from_file(const string& from)
{
    std::ifstream ifs(from + "/after.txt");

    string after;

    if (ifs.is_open())
    {
        ifs >> after;
    }

    return after;
}

void save_after_to_file(const string& where,
                        const string& content)
{
    std::ofstream ofs(where + "/after.txt",
                      std::ofstream::trunc);

    if (ofs.is_open())
    {
        ofs << content;
    }
}

string extract_image_id_from_url(const string& url)
{
    std::stringstream buff;

    bool found = false;

    auto it = url.rbegin();

    // check if url end with '/'
    if (url.back() == '/')
        ++it; // skip last character

    for (;
         it != url.rend();
         ++it)
    {
        if (*it != '/')
        {
            buff << *it;
        }
        else
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        return {};
    }

    std::string image_id = buff.str();
    std::reverse(image_id.begin(), image_id.end());

    return image_id;
}

std::ostream& operator<<(std::ostream& os, const Download_Res& dr)
{
    switch (dr)
    {
        case Download_Res::INVALID: os << "INVALID ENUM VALUE"; break;
        case Download_Res::SKIPPED: os << "SKIP"; break;
        case Download_Res::DOWNLOADED: os << "OK ( ✅ )"; break;
        case Download_Res::FAILED: os << "FAILED ( 🛑 )"; break;
        case Download_Res::UNABLE:  os << "UNABLE  ( ❌ )"; break;
    }
    return os;
}

string to_str(const Download_Res& dr)
{
    std::stringstream ss;
    ss << dr;
    return ss.str();
}

}
