#include "pch.h"

using std::cout;
using std::wcout;
using std::endl;
using std::string;
using std::wstring;
using std::vector;
using std::optional;

namespace fs = std::filesystem;

using njson = nlohmann::json;
using str_cref = const std::string&;
using vector_cref = const std::vector<nlohmann::json>&;

// TODO: 
// [] multithreading
// [] download media in the form of: https://v.redd.it/
// [] download images inside a gallery!


auto constexpr g_FILENAME_MAX_LEN = 180;
auto constexpr g_PRINT_MAX_LEN = 50;

struct Response
{
    string content;
    long code;
};

enum class Download_Res: uint8_t
{
    INVALID,
    DOWNLOADED,
    FAILED,
    SKIPPED, // file already existed
    UNABLE // don't know how to download url
};

std::ostream& operator<<(std::ostream& os, const Download_Res& dr)
{
    switch (dr)
    {
        case Download_Res::INVALID: os << "INVALID"; break;
        case Download_Res::DOWNLOADED: os << "DOWNLOADED ( ✅ )"; break;
        case Download_Res::FAILED: os << "FAILED ( 🛑 )"; break;
        case Download_Res::SKIPPED: os << "SKIPPED ( ↪ )"; break;        
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

struct Thread_Res
{
    string title;
    string url;
    Download_Res download_res = Download_Res::INVALID;
};

void run_test();

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

optional<Response> perform_request(const string& url)
{
    curlpp::Easy request;

    try
    {
        using namespace curlpp::options;

        request.setOpt<Url>(url);
        request.setOpt<UserAgent>("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/101.0.4951.64 Safari/537.36");
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
        cout <<
            std::format("[ERROR] perform_request() failed with code {}: \"{}\"",
                        static_cast<int>(e.whatCode()), (e.what()))
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
        cout << std::format("[ERROR] Json request failed for url {} with code: {}",
                            reddit_url.str(), resp.code)
            << endl;
        return {};
    }

    return resp.content;
}

bool download_file_to_disk(const string& url,
                           const string& path)
{
    std::ofstream ofs(path,
                      std::ofstream::binary |
                      std::ofstream::trunc);

    if (ofs.is_open())
    {
        auto opt_res = perform_request(url);

        if (not opt_res.has_value())
        {
            cout << std::format("[ERROR] Failed to download url '{}' to <{}>",
                                url, path) << endl;
            return false;
        }

        auto& res = opt_res.value();

        ofs << res.content;
    }
    else
    {
        cout << std::format("[WARN] Cannot open file for writing <{}>", path) << endl;
        return false;
    }

    return true;
}

bool is_extension_allowed(const string& ext)
{
    const vector<string> allowed_ext{
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

Thread_Res download_media(const njson& child,
                    const string& dest_folder)
{
    Thread_Res res;

    try
    {
        const string& raw_title = child["data"]["title"].get_ref<str_cref>();
        const string& url = child["data"]["url"].get_ref<str_cref>();

        auto filename = Utils::remove_invalid_charaters(raw_title);
        if (filename.length() > g_FILENAME_MAX_LEN)
            filename.resize(g_FILENAME_MAX_LEN);

        res.title = filename;
        res.url = url;
        //cout << std::format("[{:04}] ", counter++);

        auto ext_from_url = Utils::get_file_extension_from_url(url);
        bool can_download = is_extension_allowed(ext_from_url);

        if (!can_download)
        {
            /*cout << std::format("Cannot download '{:.{}}' url is: '{:.{}}' ( ❌ )",
                                filename, g_PRINT_MAX_LEN,
                                url, g_PRINT_MAX_LEN) << endl;*/
            res.download_res = Download_Res::UNABLE;
            return res;
        }

        auto download_path = dest_folder + "/" + filename + "." + ext_from_url;

        /*cout << std::format("Downloading '{:.{}}' to <{:.{}}> ",
                            filename, g_PRINT_MAX_LEN,
                            dest_folder, g_PRINT_MAX_LEN);*/

        if (fs::exists(download_path))
        {
            //cout << "file already downloaded! skipping..." << endl;
            res.download_res = Download_Res::SKIPPED;
            return res;
        }

        // start thread here:
        bool success = download_file_to_disk(url, download_path);

        if (success)
        {
            //cout << "( ✅ )" << endl;
            res.download_res = Download_Res::DOWNLOADED;
        }
        else
        {
            //cout << "( 🛑 )" << endl;
            res.download_res = Download_Res::FAILED;
        }
    }
    catch (const std::exception& e)
    {
        cout << std::format("[{}] {}", "download_media()", e.what()) << endl;
        return {};
    }

    return res;
}

// reddit image downloader
int rid(const string& subreddit,
        const string& when,
        const string& dest_folder)
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
                cout << std::format("[ERROR] Cannot create folder <{}>", dest_folder) << endl;
                return 1;
            }
        }

        unsigned file_processed = 0;

        while (true)
        {
            auto resp = download_json_from_reddit(subreddit,
                                                  when,
                                                  after).value();

#ifdef _DEBUG 
            // save json to disk for debugging purposes 
            {
                std::ofstream out(dest_folder + "/" + subreddit + "_" + when + "_" + after + ".json",
                                  std::ofstream::trunc |
                                  std::ofstream::binary);

                if (out.is_open())
                {
                    out << resp;
                }
            }
#endif 

            njson json = njson::parse(resp);

            if (not (json.contains("data") and
                json["data"].contains("children")))
            {
                throw std::runtime_error("Unexpected json content... 😳");
            }

            const auto& children = json["data"]["children"].get_ref<vector_cref>();

            auto count = children.size();
            cout << "Downlaoading " << count << " files" << endl;

            while (count > 0)
            {
                auto constexpr num_threads{ 10 };

                std::array<std::future<Thread_Res>, num_threads> threads;

                for (size_t i = 0;
                     i < num_threads and
                     count > 0;
                     ++i)
                {
                    auto future = std::async(std::launch::async, 
                                         download_media,
                                         std::ref(children[count - 1]),
                                         std::ref(dest_folder));
                    
                    threads[i] = std::move(future);
                    ++file_processed;
                    --count;
                }

                for (auto& thread : threads)
                {
                    if (thread.valid())
                    {
                        auto res = thread.get(); // blocking, calls wait()
                        /*cout << std::format("Cannot download '{:.{}}' url is: '{:.{}}' ( ❌ )",
                                filename, g_PRINT_MAX_LEN,
                                url, g_PRINT_MAX_LEN) << endl;*/
                        cout << std::format("[{}] {:<{}.{}} -> {}", 
                                            file_processed,
                                            res.title, g_PRINT_MAX_LEN, g_PRINT_MAX_LEN,
                                            to_str(res.download_res)) 
                            << endl;
                        /*cout << std::format("[{}] '{:.{}}' -> {}", 
                                            file_processed,
                                            res.title, g_PRINT_MAX_LEN,
                                            to_str(res.download_res)) 
                            << endl;
                        */
                        //cout << res.title << " -> " << res.download_res << endl;
                    }
                }
            }

            if (json["data"]["after"].is_null())
            {
                // nothing left do do, we download everything
                cout << "All done!" << endl;
                break;
            }

            after = json["data"]["after"].get<string>();
            save_after_to_file(dest_folder, after);
        }

        return 0;
    }
    catch (njson::parse_error& e)
    {
        cout << std::format("[parse_error] Cannot parse downloaded json from reddit.com: {}",
                            e.what()) << endl;
    }
    catch (njson::type_error& e)
    {
        cout << std::format("[type_error] Error while parsing json from reddit.com: {}",
                            e.what()) << endl;
    }
    catch (std::bad_optional_access& e)
    {
        cout << std::format("[bad_optional_access] {}", e.what()) << endl;
    }
    catch (std::runtime_error& e)
    {
        cout << std::format("[runtime_error] {}", e.what()) << endl;
    }
    catch (const std::exception& e)
    {
        cout << std::format("[exception]: {}", e.what()) << endl;
    }

    return 1;
}



// rid.exe VaporwaveAesthetics "top" [hour | day | week | month | year | all] where
//int wmain(int argc, wchar_t* argv[])
int main(int argc, char* argv[])
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    std::locale::global(std::locale("en_US.UTF-8")); // set the C/C++ locale

    run_test();

#define TESTING 0

#if TESTING

    const string subreddit = "pics"; // "VaporwaveAesthetics";
    const string when = "month"; // "day"; 
    const string dest = "picssssz"; // "🌟vaporwave-aesthetics🌟";

    return rid(subreddit, when, dest);
#else

    argparse::ArgumentParser program("rid", "1.0.0");
    program.add_description("Reddir Image Downloader\nAllows you to download all the top images from a specified subreddit");
    program.add_argument("subreddit")
        .help("subreddit name from wich all the images will be download (without the /r) for example: VaporwaveAesthetics");
    program.add_argument("when")
        .help("choose between: [hour | day | week | month | year | all]");
    program.add_argument("dest-folder")
        .help("the folder where to put all the downloaded images (the folder will be created if it doesn't exit)");
    program.add_epilog("\nNOTE: At the moment the downloadable format images are limited to: .gif .jpeg .png .jpg .bmp");

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err)
    {
        cout << err.what() << endl;
        cout << program; // print help
        return 1;
    }

    const string subreddit = program.get<string>("subreddit"); // "VaporwaveAesthetics";
    const string when = program.get<string>("when"); // "day"; 
    const string dest = program.get<string>("dest-folder"); // "🌟vaporwave-aesthetics🌟";

    return rid(subreddit, when, dest);
#endif
}

void run_test()
{
    assert(Utils::get_file_extension_from_url("https://i.imgur.com/gBj52nI.jpg") == "jpg");
    assert(Utils::get_file_extension_from_url("https://i.imgur.com/gBj52nI") == "");
    assert(Utils::get_file_extension_from_url("https://gfycat.com/MeekWeightyFrogmouth") == "");
    assert(Utils::get_file_extension_from_url("https://66.media.tumblr.com/8ead6e96ca8e3e8fe16434181e8a1493/tumblr_oruoo12vtR1s5qhggo3_1280.png") == "png");

    // gif, jpeg, png, jpg, bmp
    assert(is_extension_allowed("png"));
    assert(is_extension_allowed("bmp"));
    assert(not is_extension_allowed("mp4"));

    //auto long_title = 
    //    "Weekly vinyl rip is LIVE: This week, A class in​​.​​​.​​​. ​​CRYPTO CURRENCY "
    //    "by 猫 シ Corp. released as 7\" by Geometric Lullaby in 2021!8 tracks of "
    //    "classic mallsoft in just under 16 minutes!Link in comments < 3";
    //
    //auto clean_title = Utils::remove_invalid_charaters(long_title);

    //auto len = clean_title.length();

    //auto url = "https://external-preview.redd.it/kGZO86rtKFwRNHxTuQaPOE3XBAVwvPhXjVzZEyLntB8.jpg?auto=webp\\u0026s=8c6c31b828bbc706749dabe3ab6b3a0439480421";
}
