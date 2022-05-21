#include "pch.h"

using std::cout;
using std::wcout;
using std::endl;
using std::string;
using std::wstring;
using std::vector;
using std::optional;

namespace fs = std::filesystem;

using json = nlohmann::json;
using str_cref = const std::string&;

// TODO: better error handling in general

void run_test();

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
    std::ofstream ofs(L"after.txt",
                      std::ofstream::trunc);

    if (ofs.is_open())
    {
        ofs << after;
    }
}


struct Response
{
    string content;
    long code;
};

optional<Response> perform_request(const string& url)
{
    curlpp::Easy request;

    try
    {
        request.setOpt<curlpp::options::Url>(url);
        //request.setOpt<curlpp::options::UserAgent>("curlpp / 0.8.1 :)");
        request.setOpt<curlpp::options::FollowLocation>(true);
        request.setOpt<curlpp::options::MaxRedirs>(10);
        request.setOpt<curlpp::options::Verbose>(false);

        std::stringstream ss;
        request.setOpt<curlpp::options::WriteStream>(&ss);

        request.perform();

        auto code = curlpp::infos::ResponseCode::get(request);

        return Response{ .content = ss.str(), .code = code };
    }
    catch (const curlpp::LibcurlRuntimeError& e)
    {
        wcout <<
            std::format(L"[ERROR] perform_request() failed with code {}: \"{}\"",
                        static_cast<int>(e.whatCode()), Utils::ConvertUtf8ToWide(e.what()) )
            << endl;

        return {};
    }
}

optional<json> download_json_from_reddit(
    const string& subreddit,
    const string& when,
    const string& after = "",
    unsigned limit = 100)
{
    std::stringstream url;
    url << "https://www.reddit.com";
    url << "/r/" << subreddit;
    url << "/top.json";
    url << "?t=" << when;
    url << "&limit=" << limit;

    if (after != "")
    {
        url << "&after=" << after;
    }

    auto opt_resp = perform_request(url.str());

    if (!opt_resp.has_value())
        return {};

    auto& resp = opt_resp.value();

    if (resp.code != 200)
    {
        wcout << L"[ERROR] Json request failed for url '"
            << Utils::ConvertUtf8ToWide(url.str()) << "'"
            << L" with code: " << resp.code << endl;

        return {};
    }

#ifdef _DEBUG 
    // save json to disk for debug purposes 
    {
        std::ofstream out(L"VaporwaveAesthetics.json",
                          std::ofstream::trunc |
                          std::ofstream::binary);

        if (out.is_open())
        {
            out << resp.content;
        }
    }
#endif 

    json j;

    try
    {
        j = json::parse(resp.content);
    }
    catch (json::parse_error& e)
    {
        wcout << L"[ERROR] Cannot parse json for url '"
            << Utils::ConvertUtf8ToWide(url.str()) << L"'"
            << L" : " << Utils::ConvertUtf8ToWide(e.what()) << endl;

        return {};
    }

    return j;
}

bool download_file_to_disk(const wstring& url, 
                           const wstring& path)
{
    std::ofstream ofs(path,
                      std::ofstream::binary |
                      std::ofstream::trunc);

    if (ofs.is_open())
    {
        auto opt_res = perform_request(Utils::ConvertWideToUtf8(url));

        if (not opt_res.has_value())
        {
            wcout << std::format(L"[ERROR] Failed to download url '{}' to <{}>",
                                 url, path) << endl;
            return false;
        }

        auto& res = opt_res.value();

        ofs << res.content;
    }
    else
    {
        wcout << std::format(L"[WARN] Cannot open file for writing <{}>", path) << endl;
        return false;
    }

    return true;
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

    bool res = std::any_of(
        allowed_ext.begin(),
        allowed_ext.end(),
        [&ext](const wstring& allowed_ext)
    {
        return allowed_ext == ext;
    });

    return res;
}

// reddit image downloader
int rid(const string& subreddit,
        const string& when,
        const wstring& dest_folder)
{
    try
    {
        // 
        // get json file from r/$subreddit
        // save 'after' value to file
        // print all the title
        // get json file from r/VaporwaveAesthetics with after value

        curlpp::Cleanup cleanup;

        string after = get_after_from_file();

        if (not fs::exists(dest_folder))
        {
            if (not fs::create_directory(dest_folder))
            {
                wcout << std::format(L"[ERROR] Cannot create folder <{}>", dest_folder) << endl;
                return 1;
            }
        }

        unsigned counter = 0;

        while (true)
        {
            auto opt_json = download_json_from_reddit(subreddit, when,
                                                      after);

            if (!opt_json)
                return 1;

            auto& json = opt_json.value();

            if (not (json.contains("data") and
                json["data"].contains("children")))
            {
                wcout << L"[ERROR] Unexpected json content... 😳" << endl;
                return 1;
            }

            const auto& children = json["data"]["children"];

            for (const auto& child : children)
            {
                ++counter;

                const auto& data = child["data"];

                wstring raw_title = Utils::ConvertUtf8ToWide(data["title"].get_ref<str_cref>());
                wstring url   = Utils::ConvertUtf8ToWide(data["url"].get_ref<str_cref>());
                //const auto& id = data["id"].get_ref<str_cref>();
                //const auto& domain = data["domain"].get_ref<str_cref>();

                auto ext_from_url = Utils::get_file_extension_from_url(url);

                bool can_download = is_extension_allowed(ext_from_url);
                //(data["is_video"] == false) and
                //(data["domain"] == "i.redd.it");

                //wcout << L"[INFO] Downloading '" << raw_title << L"' ";
                wcout << std::format(L"[{:04}] ", counter);

                if (can_download)
                {
                    auto filename = Utils::remove_invalid_charaters(raw_title);
                    auto path = dest_folder + L"/" + filename + L"." + ext_from_url;

                    wcout << std::format(L"Downloading '{}' ({}) to <{}> ", 
                                         raw_title, url, path);

                    if (fs::exists(path))
                    {
                        wcout << L"file already downloaded! skipping..." << endl;
                        continue;
                    }
                    
                    auto success = download_file_to_disk(url, path);

                    if (success)
                    {
                        wcout << L"( ✅ )" << endl;
                        //wcout << L"( ✓ )" << endl;
                    }
                    else
                    {
                        wcout << L"( 🛑 )" << endl;
                    }
                }
                else
                {
                    wcout << std::format(L"Can't download url '{}' ( ❌ )", url) << endl;
                }

                using namespace std::chrono_literals;
                std::this_thread::sleep_for(800ms);
            }

            if (json["data"]["after"].is_null())
            {
                // nothing left do do, leaving
                wcout << "All done!" << endl;
                break;
            }

            after = json["data"]["after"].get<string>();
            save_after_to_file(after);
        }

    }
    catch (const std::exception& e)
    {
        cout << "\n\n---" << endl;
        cout << "[EXCEPTION]: " << e.what() << endl;
        cout << "---\n\n" << endl;
        int stop_here = 0;
    }

    return 0;
}

// rid.exe VaporwaveAesthetics "top" [hour | day | week | month | year | all] where
int wmain(int argc, wchar_t* argv[])
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    std::locale::global(std::locale("en_US.UTF-8")); // set the C/C++ locale

    run_test();

    if (argc != 4)
    {
        wcout << L"Bro i need 3 parameter..." << endl;
        return 1;
    }

    const string subreddit = Utils::ConvertWideToUtf8(argv[1]); // "VaporwaveAesthetics";
    const string when = Utils::ConvertWideToUtf8(argv[2]); // "day"; 
    const wstring dest = argv[3]; // L"🌟vaporwave-aesthetics🌟";

    return rid(subreddit, when, dest);
}

void run_test()
{
    assert(Utils::get_file_extension_from_url(L"https://i.imgur.com/gBj52nI.jpg") == L"jpg");
    assert(Utils::get_file_extension_from_url(L"https://i.imgur.com/gBj52nI") == L"");
    assert(Utils::get_file_extension_from_url(L"https://gfycat.com/MeekWeightyFrogmouth") == L"");
    assert(Utils::get_file_extension_from_url(L"https://66.media.tumblr.com/8ead6e96ca8e3e8fe16434181e8a1493/tumblr_oruoo12vtR1s5qhggo3_1280.png") == L"png");

    // gif, jpeg, png, jpg, bmp
    assert(is_extension_allowed(L"png"));
    assert(is_extension_allowed(L"bmp"));
    assert(not is_extension_allowed(L"mp4"));
}
