#include "pch.h"

using std::cout;
using std::wcout;
using std::endl;
using std::string;
using std::wstring;


using json = nlohmann::json;
using str_refc = const std::string&;

json get_json_from_reddit(const string& after = "", unsigned limit = 10)
{
    // Our request to be sent.
    curlpp::Easy request;

    const string base_url = "https://www.reddit.com/r/VaporwaveAesthetics/top.json?t=all";

    string url;
    if (after != "")
        url = std::format("{}&limit={}&after={}",
        base_url, limit, after);
    else
        url = std::format("{}&limit={}",
        base_url, limit);

    request.setOpt<curlpp::options::Url>(url);
    /*request.setOpt<curlpp::options::UserAgent>("stocazzo");
    request.setOpt<curlpp::options::MaxRedirs>(10);
    request.setOpt<curlpp::options::FollowLocation>(true);*/
    request.setOpt<curlpp::options::Verbose>(false);

    std::stringstream ss;
    request.setOpt<curlpp::options::WriteStream>(&ss);

    request.perform();

    return json::parse(ss.str());

    //{
//    std::ofstream out(L"VaporwaveAesthetics-top.json",
//        std::ofstream::trunc);

//    if (out.is_open())
//    {
//        out << ss.rdbuf();
//    }
//}
}

string get_after_from_file()
{
    std::ifstream ifs(L"after.txt");

    string after;

    if (ifs.is_open())
    {
        ifs >> after;
    }

    return after;
}

void save_after_to_file(const string& after)
{
    std::ofstream ofs(L"after.txt", std::ofstream::trunc);

    if (ofs.is_open())
    {
        ofs << after;
    }
}

void download_file(const string& url, const string& dest_file)
{
    auto get_extension = [](const string& from)
    {
        std::stringstream buff;

        for (auto it = from.rbegin();
            it != from.rend();
            ++it)
        {
            if (*it != '.')
                buff << *it;
            else
                break;
        }

        string ext = buff.str();
        std::reverse(ext.begin(), ext.end());
        return ext;
    };

    curlpp::Easy request;

    request.setOpt<curlpp::options::Url>(url);
    request.setOpt<curlpp::options::Verbose>(false);

    wstring w_dest_file = Utils::ConvertUtf8ToWide(dest_file);
    w_dest_file += L"." + Utils::ConvertUtf8ToWide(get_extension(url));

    std::ofstream ofs(w_dest_file,
        std::ofstream::binary |
        std::ofstream::trunc);

    if (ofs.is_open())
    {
        request.setOpt<curlpp::options::WriteStream>(&ofs);
        request.perform();
    }
    else
    {
        cout << "[WARN] cannot download file: " << url << endl;
    }
}

int main(int, char**)
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::locale::global(std::locale("en_US.UTF-8")); // set the C++ and C locale

    try
    {

        // get json file from r/VaporwaveAesthetics
        // save 'after' value to file
        // print all the title
        // get json file from r/VaporwaveAesthetics with after value

        // 
        // That's all that is needed to do cleanup of used resources (RAII style).
        curlpp::Cleanup cleanup;

        string after = get_after_from_file();
        unsigned counter = 1;

        while (true)
        {
            auto json = get_json_from_reddit(after);

            after = json["data"]["after"].get<string>();

            save_after_to_file(after);

            auto& children = json["data"]["children"];

            for (const auto& child : children)
            {
                const auto& data = child["data"];

                const auto& title = data["title"].get_ref<str_refc>();
                const auto& url = data["url"].get_ref<str_refc>();
                //const auto& id = data["id"].get_ref<str_refc>();
                const auto& domain = data["domain"].get_ref<str_refc>();

                bool can_download =
                    (data["is_video"] == false) and
                    (data["domain"] == "i.redd.it");

                std::wstring msg;
                if (can_download)
                {
                    msg = std::format(L"[{}] - {} ( ✓ )",
                        counter++, Utils::ConvertUtf8ToWide(title));

                    download_file(url, title);
                }
                else
                {
                    msg = std::format(L"[{}] - {} ( ❌ ) [{}]",
                        counter++, Utils::ConvertUtf8ToWide(title),
                        std::wstring(url.begin(), url.end())
                    );
                }

                wcout << msg << endl;
            }
        }

    }
    catch (std::exception& e)
    {
        cout << "[EXC]: " << e.what() << endl;
        int stop_here = 0;
    }

    return 0;
}

