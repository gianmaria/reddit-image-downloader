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

// TODO: 
// [] multithreading
// [] download images inside a gallery!


const size_t g_FILENAME_MAX_LEN = 180;

struct Response
{
    string content;
    long code;
};

void run_test();

string get_after_from_file(const wstring& from)
{
    std::ifstream ifs(from + L"/after.txt");

    string after;

    if (ifs.is_open())
    {
        ifs >> after;
    }

    return after;
}

void save_after_to_file(const wstring& where,
                        const string& content)
{
    std::ofstream ofs(where + L"/after.txt",
                      std::ofstream::trunc);

    if (ofs.is_open())
    {
        ofs << content;
    }
}

optional<Response> perform_request(const string& url)
{
    curlpp::Easy request;

    try
    {
        using namespace curlpp::options;

        request.setOpt<Url>(url);
        //request.setOpt<UserAgent>("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/101.0.4951.64 Safari/537.36");
        request.setOpt<FollowLocation>(true);
        request.setOpt<MaxRedirs>(10);
        request.setOpt<Verbose>(false);

        std::stringstream ss;
        request.setOpt<WriteStream>(&ss);

        request.perform();

        auto code = curlpp::infos::ResponseCode::get(request);

        return Response{ .content = ss.str(), .code = code };
    }
    catch (const curlpp::LibcurlRuntimeError& e)
    {
        wcout <<
            std::format(L"[ERROR] perform_request() failed with code {}: \"{}\"",
                        static_cast<int>(e.whatCode()), Utils::ConvertUtf8ToWide(e.what()))
            << endl;

        return {};
    }
}

optional<string> download_json_from_reddit(
    const string& subreddit,
    const string& when,
    const string& after = "",
    unsigned limit = 100)
{
    std::stringstream reddit_url;
    reddit_url << "https://www.reddit.com";
    reddit_url << "/r/" << subreddit;
    reddit_url << "/top.json";
    reddit_url << "?t=" << when;
    reddit_url << "&raw_json=1";
    reddit_url << "&limit=" << limit;

    if (after != "")
    {
        reddit_url << "&after=" << after;
    }

    auto opt_resp = perform_request(reddit_url.str());

    if (!opt_resp.has_value())
        return {};

    auto& resp = opt_resp.value();

    if (resp.code != 200)
    {
        wcout << std::format(L"[ERROR] Json request failed for url {} with code: {}",
                             Utils::ConvertUtf8ToWide(reddit_url.str()), resp.code)
            << endl;
        return {};
    }

    return resp.content;
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

        string after = get_after_from_file(dest_folder);

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
            auto opt_resp = download_json_from_reddit(subreddit, when,
                                                      after);

            if (!opt_resp)
                return 1;

            auto& resp = opt_resp.value();

#ifdef _DEBUG 
            // save json to disk for debugging purposes 
            {
                std::ofstream out(subreddit + "_" + when + "_" + after + ".json",
                                  std::ofstream::trunc |
                                  std::ofstream::binary);

                if (out.is_open())
                {
                    out << resp;
                }
            }
#endif 

            json json;

            try
            {
                json = json::parse(resp);
            }
            catch (json::parse_error& e)
            {
                wcout << std::format(L"[ERROR] Cannot parse downloaded json from reddit.com: {}",
                                     Utils::ConvertUtf8ToWide(e.what()));
                return 1;
            }

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
                wstring url = Utils::ConvertUtf8ToWide(data["url"].get_ref<str_cref>());
                //const auto& id = data["id"].get_ref<str_cref>();
                //const auto& domain = data["domain"].get_ref<str_cref>();

                auto ext_from_url = Utils::get_file_extension_from_url(url);

                bool can_download = is_extension_allowed(ext_from_url);
                //(data["is_video"] == false) and
                //(data["domain"] == "i.redd.it");

                //wcout << L"[INFO] Downloading '" << raw_title << L"' ";
                wcout << std::format(L"[{:04}] ", counter);

                auto filename = Utils::remove_invalid_charaters(raw_title);
                if (filename.length() > g_FILENAME_MAX_LEN)
                    filename.resize(g_FILENAME_MAX_LEN);

                if (can_download)
                {
                    auto path = dest_folder + L"/" + filename + L"." + ext_from_url;

                    wcout << std::format(L"Downloading '{}' ({}) to <{}> ",
                                         filename, url, path);

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
                    wcout << std::format(L"Cannot download '{}' url '{}' ( ❌ )",
                                         filename, url) << endl;
                }

                using namespace std::chrono_literals;
                std::this_thread::sleep_for(100ms);
            }

            if (json["data"]["after"].is_null())
            {
                // nothing left do do, we download everything
                wcout << "All done!" << endl;
                break;
            }

            after = json["data"]["after"].get<string>();
            save_after_to_file(dest_folder, after);
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

#define TESTING 0

#if TESTING

    const string subreddit = "pics"; // "VaporwaveAesthetics";
    const string when = "month"; // "day"; 
    const wstring dest = L"picssssz"; // L"🌟vaporwave-aesthetics🌟";

    return rid(subreddit, when, dest);
#else
    if (argc != 4)
    {
        wcout << L"Bro i need 3 parameter..." << endl;
        return 1;
}

    const string subreddit = Utils::ConvertWideToUtf8(argv[1]); // "VaporwaveAesthetics";
    const string when = Utils::ConvertWideToUtf8(argv[2]); // "day"; 
    const wstring dest = argv[3]; // L"🌟vaporwave-aesthetics🌟";

    return rid(subreddit, when, dest);
#endif
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

    //auto long_title = 
    //    L"Weekly vinyl rip is LIVE: This week, A class in​​.​​​.​​​. ​​CRYPTO CURRENCY "
    //    L"by 猫 シ Corp. released as 7\" by Geometric Lullaby in 2021!8 tracks of "
    //    L"classic mallsoft in just under 16 minutes!Link in comments < 3";
    //
    //auto clean_title = Utils::remove_invalid_charaters(long_title);

    //auto len = clean_title.length();

    //auto url = "https://external-preview.redd.it/kGZO86rtKFwRNHxTuQaPOE3XBAVwvPhXjVzZEyLntB8.jpg?auto=webp\\u0026s=8c6c31b828bbc706749dabe3ab6b3a0439480421";

    int stop = 0;
}
