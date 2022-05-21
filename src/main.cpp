#include "pch.h"

using std::cout;
using std::wcout;
using std::endl;
using std::string;
using std::wstring;
using std::vector;

namespace fs = std::filesystem;

using json = nlohmann::json;
using str_refc = const std::string&;

// TODO: better error handling in general

std::optional<json> download_json_from_reddit(const string& after = "",
                                              unsigned limit = 100)
{
    std::stringstream ss;

    // Our request to be sent.
    curlpp::Easy request;

    ss << "https://www.reddit.com/";
    ss << "r/VaporwaveAesthetics/";
    ss << "top.json";
    ss << "?t=all";
    ss << "&limit=" << limit;

    if (after != "")
    {
        ss << "&after=" << after;
    }

    request.setOpt<curlpp::options::Url>(ss.str());
    //request.setOpt<curlpp::options::UserAgent>("curlpp / 0.8.1 :)");
    request.setOpt<curlpp::options::MaxRedirs>(10);
    request.setOpt<curlpp::options::FollowLocation>(true);
    request.setOpt<curlpp::options::Verbose>(false);

    ss.str({});
    ss.clear();
    request.setOpt<curlpp::options::WriteStream>(&ss);

    request.perform();

    {
        std::ofstream out(L"VaporwaveAesthetics.json",
                          std::ofstream::trunc);

        if (out.is_open())
        {
            out << ss.str();
        }
    }

    try
    {
        return json::parse(ss);
    }
    catch (json::parse_error& e)
    {
        cout << "❌ " << e.what() << endl;
        cout << "Downloaded json:" << endl;
        cout << ss.str() << endl;
        return {};
    }
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

void download_file_to_disk(const wstring& url, wstring dest_filepath)
{
    curlpp::Easy request;

    request.setOpt<curlpp::options::Url>(Utils::ConvertWideToUtf8(url));
    //request.setOpt<curlpp::options::UserAgent>("curlpp / 0.8.1 :)");
    request.setOpt<curlpp::options::MaxRedirs>(10);
    request.setOpt<curlpp::options::FollowLocation>(true);
    request.setOpt<curlpp::options::Verbose>(false);

    Utils::remove_invalid_charaters(dest_filepath);
    dest_filepath = L"data\\" + dest_filepath + L"." + Utils::get_file_extension_from_url(url);

    if (not fs::exists(L"data\\"))
        fs::create_directory(L"data");

    if (fs::exists(dest_filepath))
    {
        wcout << L"[⚠️] file already downloaded, skipping... '" << dest_filepath << L"'" << endl;
        return;
    }

    std::ofstream ofs(dest_filepath,
                      std::ofstream::binary |
                      std::ofstream::trunc);

    if (ofs.is_open())
    {
        request.setOpt<curlpp::options::WriteStream>(&ofs);
        request.perform();
        
        // TODO: handle error code when resources not available...
        auto response_code = curlpp::infos::ResponseCode::get(request);

        if (response_code != 200)
        {
            throw std::runtime_error("Response code is: " + std::to_string(response_code));
        }
    }
    else
    {
        wcout << L"[⚠️] cannot open file for writing '" << dest_filepath << L"'" << endl;
    }
}

bool is_extension_allowed(const wstring& ext)
{
    const vector<wstring> allowed_ext{
        L"gif",
        L"jpeg",
        L"png",
        L"jpg",
        L"bmp"
    };

    return std::any_of(
        allowed_ext.begin(),
        allowed_ext.end(),
        [&ext](const wstring& allowed_ext)
    {
        return allowed_ext == ext;
    });

}

int program()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::locale::global(std::locale("en_US.UTF-8")); // set the C++ and C locale

    try
    {
        // 
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
            auto opt_json = download_json_from_reddit(after);

            if (!opt_json)
                break; // sorry we can't continue

            auto json = opt_json.value();

            auto& children = json["data"]["children"];

            for (const auto& child : children)
            {
                const auto& data = child["data"];

                wstring title = Utils::ConvertUtf8ToWide(data["title"].get_ref<str_refc>());
                wstring url = Utils::ConvertUtf8ToWide(data["url"].get_ref<str_refc>());
                //const auto& id = data["id"].get_ref<str_refc>();
                //const auto& domain = data["domain"].get_ref<str_refc>();

                bool can_download = is_extension_allowed(Utils::get_file_extension_from_url(url));
                //(data["is_video"] == false) and
                //(data["domain"] == "i.redd.it");

                std::wstring msg;
                if (can_download)
                {
                    msg = std::format(L"[{}] - {} ( ✓ )",
                                      counter++, title);

                    download_file_to_disk(url, title);
                }
                else
                {
                    msg = std::format(L"[{}] - {} ( ❌ ) [{}]",
                                      counter++, title,
                                      std::wstring(url.begin(), url.end())
                    );
                }

                wcout << msg << endl;

                using namespace std::chrono_literals;
                std::this_thread::sleep_for(500ms);
            }

            if (json["data"]["after"].is_null())
            {
                wcout << "✅ All done!" << endl;
                break;
            }
            after = json["data"]["after"].get<string>();
            save_after_to_file(after);
        }

    }
    catch (std::exception& e)
    {
        cout << "[EXC]: " << e.what() << endl;
        int stop_here = 0;
    }

    return 0;
}

int main_test()
{
    assert(Utils::get_file_extension_from_url(L"https://i.imgur.com/gBj52nI.jpg") == L"jpg");
    assert(Utils::get_file_extension_from_url(L"https://i.imgur.com/gBj52nI") == L"");
    assert(Utils::get_file_extension_from_url(L"https://gfycat.com/MeekWeightyFrogmouth") == L"");
    assert(Utils::get_file_extension_from_url(L"https://66.media.tumblr.com/8ead6e96ca8e3e8fe16434181e8a1493/tumblr_oruoo12vtR1s5qhggo3_1280.png") == L"png");

    // gif, jpeg, png, jpg, bmp
    assert(is_extension_allowed(L"png"));
    assert(is_extension_allowed(L"bmp"));
    assert(not is_extension_allowed(L"mp4"));
    return 0;
}

int main()
{
    main_test();
    program();
    return 0;
}