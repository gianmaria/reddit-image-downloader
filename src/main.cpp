#include "pch.h"

#include "utils.h"

// TODO: 
// [X] multithreading
// [] download images from website other than reddit
//    [X] imgur.com
//    [] v.redd.it
//    [] gfycat.com
// [] download images inside a gallery! e.g. https://www.reddit.com/gallery/uw2ikr


auto constexpr g_TITLE_MAX_LEN = 120;
auto constexpr g_PRINT_MAX_LEN = 50;
auto constexpr g_num_threads{ 1 }; // num threads

struct Response
{
    string content;
    long code;
};

enum class Download_Res : uint8_t
{
    INVALID,
    DOWNLOADED,
    FAILED,
    SKIPPED, // file already downloaded
    UNABLE // don't know how to download url
};

std::ostream& operator<<(std::ostream& os, const Download_Res& dr)
{
    switch (dr)
    {
        case Download_Res::INVALID: os << "INVALID ENUM VALUE"; break;
        case Download_Res::SKIPPED:
        case Download_Res::DOWNLOADED: os << "( ✅ )"; break;
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

struct Thread_Res
{
    long file_id = -1;
    string title = "null";
    string url = "null";
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

optional<Response> perform_request(const string& url,
                                   const std::list<string>& headers = {})
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
        request.setOpt<HttpHeader>(headers);

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
                           const string& destination)
{
    std::ofstream ofs(destination,
                      std::ofstream::binary |
                      std::ofstream::trunc);

    if (ofs.is_open())
    {
        auto opt_res = perform_request(url);

        if (not opt_res.has_value())
        {
            return false;
        }

        auto& res = opt_res.value();

        ofs << res.content;
    }
    else
    {
        cout << std::format("[WARN] Cannot open file for writing <{}>", destination) << endl;
        return false;
    }

    return true;
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


std::vector<string> handle_imgur(string_cref subreddit,
                                 string_cref image_id)
{
    // https://apidocs.imgur.com/#10456589-7167-4b5c-acd3-a1e4eb6a95ed
    // api endpoint:
    // https://api.imgur.com/3/gallery/r/{{subreddit}}/{{subredditImageId}}

    string url = "https://api.imgur.com/3/gallery/r/" + subreddit + "/" + image_id;

    auto opt_resp = perform_request(url,
                                    { "Authorization: Client-ID " +
                                    Utils::env("IMGUR_CLIENT_ID") });

    if (not opt_resp.has_value())
    {
        return {};
    }

    auto content = opt_resp.value().content;

    njson json;

    try
    {
        json = njson::parse(content);
    }
    catch (const njson::parse_error& e)
    {
        cout << "[ERROR] malformed json response from imgur.com: " << e.what() << endl;
        return {};
    }

    if (not json["success"].get<bool>())
    {
        cout << "[ERROR] returned from imgur.com contains an error, status: "
            << json["status"] << endl;
        return {};
    }

    std::vector<string> res;

    if (json["data"].contains("images"))
    {
        const auto& images = json["data"]["images"];

        for (const auto& image : images)
        {
            res.push_back(image["link"]);
        }
    }
    else
    {
        res.push_back(json["data"]["link"]);
    }

    return res;
}

Thread_Res download_media(long file_id,
                          const njson& child,
                          const string& dest_folder)
{
    auto perform_download = [](string_cref url,
                               string_cref title,
                               string_cref dest_folder
                               ) -> Download_Res
    {
        auto ext_from_url = Utils::get_file_extension_from_url(url);

        auto file_path = dest_folder + "\\" + title + "." + ext_from_url;

        if (fs::exists(file_path))
        {
            return Download_Res::SKIPPED;
        }

        bool success = download_file_to_disk(url, file_path);

        if (success)
        {
            return Download_Res::DOWNLOADED;
        }
        else
        {
            return Download_Res::FAILED;
        }
    };

    Thread_Res res;

    try
    {
        res.file_id = file_id;

        const string& raw_title = child["data"]["title"].get_ref<str_cref>();

        // TMP:
        /*
        * example with very long title to cut
        if (child["data"]["url"].get_ref<str_cref>() == "https://i.redd.it/etoxbqvodjt31.png")
            int stop = 0;
        */

        auto title = Utils::remove_invalid_charaters(raw_title);

        if (Utils::UTF8_len(title) > g_TITLE_MAX_LEN)
        {
            // TODO: use proper resizing...
            Utils::resize_string(title, g_TITLE_MAX_LEN);
        }

        res.title = title;

        const string& url = child["data"]["url"].get_ref<str_cref>();
        res.url = url;


        if (Utils::get_file_extension_from_url(url) != "")
        {
            // try direct download
            res.download_res = perform_download(url, title, dest_folder);
        }
        else
        {
            const string& domain = child["data"]["domain"].get_ref<str_cref>();

            if (domain == "v.redd.it")
            {
                // TODO:
                res.download_res = Download_Res::UNABLE;
            }
            else if (domain == "imgur.com")
            {
                const string& subreddit = child["data"]["subreddit"].get_ref<str_cref>();
                string image_id = extract_image_id_from_url(url);

                auto actual_urls = handle_imgur(subreddit, image_id);

                std::for_each(actual_urls.begin(),
                              actual_urls.end(),
                              [&title, &dest_folder, &res, &perform_download](string_cref actual_url)
                {
                    // TODO: this is problematic.... multiple downloads and only one result....
                    res.download_res = perform_download(actual_url, title, dest_folder);
                });
            }
            else if (domain == "gfycat.com")
            {
                // https://developers.gfycat.com/api/?curl#getting-info-for-a-single-gfycat
                // ecample: https://api.gfycat.com/v1/gfycats/JampackedUnrulyArcherfish
                // TODO:
                res.download_res = Download_Res::UNABLE;
            }
            else
            {
                res.download_res = Download_Res::UNABLE;
            }
        }
    }
    catch (const std::exception& e)
    {
        cout << std::format("[ECXEP][download_media()] {} url: {}",
                            e.what(), child["data"]["url"].get_ref<str_cref>())
            << endl;
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
        curlpp::Cleanup cleanup;

        string after = get_after_from_file(dest_folder);

        if (not fs::exists(dest_folder))
        {
            if (not fs::create_directories(dest_folder))
            {
                cout << std::format("[ERROR] Cannot create folder <{}>", dest_folder) << endl;
                return 1;
            }
        }

        long files_processed = 0;

        while (true)
        {
            auto resp = download_json_from_reddit(subreddit,
                                                  when,
                                                  after).value();

#ifdef _DEBUG 
            // save json to disk for debugging purposes 
            {
                std::ofstream out(dest_folder + "/!#_" + subreddit 
                                  + "_" + when + "_" + after + ".json",
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

            auto files_to_download = children.size();
            //cout << "[INFO] Downlaoading " << file_to_download << " files" << endl;

            while (files_to_download > 0)
            {
                std::array<std::future<Thread_Res>, g_num_threads> threads;

                for (size_t i = 0;
                     i < g_num_threads and
                     files_to_download > 0;
                     ++i)
                {
                    ++files_processed;
                    auto future = std::async(std::launch::async,
                                             download_media,
                                             files_processed,
                                             std::ref(children[files_to_download - 1]),
                                             std::ref(dest_folder));

                    threads[i] = std::move(future);
                    --files_to_download;
                }

                for (auto& thread : threads)
                {
                    if (thread.valid())
                    {
                        auto res = thread.get(); // blocking, calls wait()
                        if (res.download_res == Download_Res::DOWNLOADED or
                            res.download_res == Download_Res::SKIPPED)
                        {
                            cout << std::format("[{}] {:<{}.{}} -> {}",
                                                res.file_id,
                                                res.title, g_PRINT_MAX_LEN, g_PRINT_MAX_LEN,
                                                to_str(res.download_res)) << endl;
                        }
                        else
                        {
                            cout << std::format("[{}] {:<{}.{}} -> {} url: '{}'",
                                                res.file_id,
                                                res.title, g_PRINT_MAX_LEN, g_PRINT_MAX_LEN,
                                                to_str(res.download_res),
                                                res.url) << endl;
                        }
                    }
                }
            }

            if (json["data"]["after"].is_null())
            {
                // nothing left do do, we download everything
                cout << "All done!" << endl;
                fs::remove(dest_folder + "/after.txt");
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
    program.add_description("Reddit Image Downloader\nAllows you to download all the top images from a specified subreddit");
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

    assert(Utils::is_extension_allowed(Utils::get_file_extension_from_url("https://i.redd.it/nqa4sfb8ns191.png")));


    // gif, jpeg, png, jpg, bmp
    assert(Utils::is_extension_allowed("png"));
    assert(Utils::is_extension_allowed("bmp"));
    assert(not Utils::is_extension_allowed("mp4"));

    //auto long_title = 
    //    "Weekly vinyl rip is LIVE: This week, A class in​​.​​​.​​​. ​​CRYPTO CURRENCY "
    //    "by 猫 シ Corp. released as 7\" by Geometric Lullaby in 2021!8 tracks of "
    //    "classic mallsoft in just under 16 minutes!Link in comments < 3";
    //
    //auto clean_title = Utils::remove_invalid_charaters(long_title);

    //auto len = clean_title.length();

    //auto url = "https://external-preview.redd.it/kGZO86rtKFwRNHxTuQaPOE3XBAVwvPhXjVzZEyLntB8.jpg?auto=webp\\u0026s=8c6c31b828bbc706749dabe3ab6b3a0439480421";

    auto GIPY_CLIENT_ID = Utils::env("GIPY_CLIENT_ID");
    auto IMGUR_CLIENT_ID = Utils::env("IMGUR_CLIENT_ID");
    auto REDDIT_CLIENT_ID = Utils::env("REDDIT_CLIENT_ID");

    int stop = 0;
}
