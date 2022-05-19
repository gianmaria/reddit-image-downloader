#include "pch.h"

using std::cout;
using std::wcout;
using std::endl;
using std::string;
using std::wstring;


using json = nlohmann::json;
using str_refc = const std::string&;

json get_json_from_reddit(const string& after = "")
{
    return {};
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

        // Our request to be sent.
        curlpp::Easy request;

        // Set the URL.
        request.setOpt<curlpp::options::Url>("https://www.reddit.com/r/VaporwaveAesthetics/top.json?t=all&limit=30");
        /*request.setOpt<curlpp::options::UserAgent>("stocazzo");
        request.setOpt<curlpp::options::MaxRedirs>(10);
        request.setOpt<curlpp::options::FollowLocation>(true);*/
        request.setOpt<curlpp::options::Verbose>(false);

        std::stringstream ss;
        request.setOpt<curlpp::options::WriteStream>(&ss);

        request.perform();

        {
            std::ofstream out(L"VaporwaveAesthetics-top.json",
                std::ofstream::trunc);

            if (out.is_open())
            {
                out << ss.rdbuf();
            }
        }

        auto json = json::parse(ss.str());

        auto& after = json["data"]["after"];
        auto& children = json["data"]["children"];

        unsigned index = 1;
        for (const auto& child : children)
        {
            const auto& data = child["data"];

            const auto& title = data["title"].get_ref<str_refc>();
            const auto& link = data["url"].get_ref<str_refc>();
            const auto& id = data["id"].get_ref<str_refc>();
            const auto& domain = data["domain"].get_ref<str_refc>();

            bool can_download = (data["is_video"] == false);

            std::wstring msg;
            if (can_download)
            {
                msg = std::format(L"[{}] - {} ( ✓ )",
                    index++, Utils::ConvertUtf8ToWide(title));
            }
            else
            {
                msg = std::format(L"[{}] - {} ( ❌ ) [{}]",
                    index++, Utils::ConvertUtf8ToWide(title),
                    std::wstring(link.begin(), link.end())
                );
            }

            wcout << msg << endl;
        }

    }
    catch (std::exception& e)
    {
        cout << "[EXC]: " << e.what() << endl;
        int stop_here = 0;
    }

    return 0;
}

