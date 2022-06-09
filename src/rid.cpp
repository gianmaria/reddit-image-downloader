#include "pch.h"

#include "rid.h"
#include "utils.h"
#include "test.h"

/* 

TODO:
[X] multithreading
 [] download images from website other than reddit
    [X] imgur.com
    [] i.imgur.com
    [] v.redd.it
    [x] gfycat.com
 [] download images inside a gallery! e.g. https://www.reddit.com/gallery/uw2ikr
 
 */

optional<Response> perform_request(const string& url,
                                   const std::list<string>& headers)
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
    const string& after,
    unsigned limit)
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


Download_Res perform_download(string_cref url,
                              string_cref title,
                              string_cref dest_folder)
{
    auto ext_from_url = Utils::get_file_extension_from_url(url);

    auto file_path = dest_folder + "\\" + title + "." + ext_from_url;

    if (fs::exists(file_path))
    {
        return Download_Res::SKIPPED;
    }

    std::ofstream ofs(file_path,
                      std::ofstream::binary |
                      std::ofstream::trunc);

    if (not ofs.is_open())
    {
        cout << std::format("[WARN] Cannot open file for writing <{}>", file_path) << endl;
        return Download_Res::FAILED;
    }

    auto opt_res = perform_request(url);

    if (not opt_res.has_value())
    {
        return Download_Res::FAILED;
    }

    auto& res = opt_res.value();

    ofs << res.content;

    return Download_Res::DOWNLOADED;
}

Download_Res handle_imgur(string_cref subreddit,
                          string_cref url,
                          string_cref title,
                          string_cref dest_folder)
{
    // https://apidocs.imgur.com/#10456589-7167-4b5c-acd3-a1e4eb6a95ed
    // api endpoint:
    // https://api.imgur.com/3/gallery/r/{{subreddit}}/{{subredditImageId}}

    try
    {
        // TODO: handle gifv
        string image_id = Utils::extract_image_id_from_url(url);

        string api_endpoint = "https://api.imgur.com/3/gallery/r/" + subreddit + "/" + image_id;

        auto imgur_client_id = Utils::env("IMGUR_CLIENT_ID");

        if (imgur_client_id == "")
        {
            cout << "[WARN] no client id for imgur inside file .env" << endl;
            return Download_Res::FAILED;
        }

        auto opt_resp = perform_request(api_endpoint,
                                        { "Authorization: Client-ID " +
                                         imgur_client_id });

        if (not opt_resp.has_value())
        {
            return Download_Res::FAILED;
        }

        auto content = opt_resp.value().content;

        njson json = njson::parse(content);

        if (not json["success"].get<bool>())
        {
            cout << "[ERROR] json returned from imgur.com contains an error, status: "
                << json["status"] << endl;
            return Download_Res::FAILED;
        }

        std::vector<string> urls;

        if (json["data"].contains("images"))
        {
            const auto& images = json["data"]["images"];

            for (const auto& image : images)
            {
                urls.push_back(image["link"]);
            }
        }
        else
        {
            urls.push_back(json["data"]["link"]);
        }

        std::for_each(urls.begin(),
                      urls.end(),
                      [&title, &dest_folder](string_cref actual_url)
        {
            // TODO: this is problematic.... multiple downloads and only one result....
            auto res = perform_download(actual_url, title, dest_folder);
        });

        return Download_Res::DOWNLOADED;
    }
    catch (const njson::parse_error& e)
    {
        cout << "[ERROR] malformed json response from imgur.com: " << e.what() << endl;
        return Download_Res::FAILED;
    }
}

Download_Res handle_gfycat(
    string_cref url,
    string_cref title,
    string_cref dest_folder)
{
    try
    {
        // https://developers.gfycat.com/api/?curl#getting-info-for-a-single-gfycat
        // example: https://api.gfycat.com/v1/gfycats/JampackedUnrulyArcherfish

        Download_Res res = Download_Res::FAILED;

        string image_id = Utils::extract_image_id_from_url(url);

        if (image_id.find_first_of('-') != string::npos)
        {
            // NOTE: does this always holds true?
            image_id = image_id.substr(0, image_id.find_first_of('-'));
        }

        const string api_endpoint = "https://api.gfycat.com/v1/gfycats/" + image_id;

        auto resp = perform_request(api_endpoint);

        njson json;
        json = njson::parse(resp.value().content);

        string* actual_url = nullptr;

        if (json.contains("gfyItem"))
        {
            if (json["gfyItem"].contains("url"))
            {
                actual_url = json["gfyItem"]["url"].get_ptr<string*>();
            }
            else if (json["gfyItem"].contains("mp4Url"))
            {
                actual_url = json["gfyItem"]["mp4Url"].get_ptr<string*>();
            }
            else
            {
                int stop = 0;
            }
        }
        else if (json.contains("errorMessage"))
        {
            int stop = 0;
        }

        if (actual_url)
        {
            res = perform_download(*actual_url, title, dest_folder);
        }

        return res;
    }
    catch (const njson::parse_error& e)
    {
        cout << "Error while parsing gfycat response json: " << e.what() << endl;
        return Download_Res::FAILED;
    }
}

Thread_Res download_media(long file_id,
                          const njson& child,
                          const string& dest_folder)
{
    try
    {
        Thread_Res res;
        res.file_id = file_id;

        const string& raw_title = child["data"]["title"].get_ref<str_cref>();

        auto title = Utils::remove_invalid_charaters(raw_title);

        if (Utils::UTF8_len(title) > g_TITLE_MAX_LEN)
        {
            Utils::resize_string(title, g_TITLE_MAX_LEN);
        }

        res.title = title;

        const string& url = child["data"]["url"].get_ref<str_cref>();
        res.url = url;

        if (Utils::get_file_extension_from_url(url) != "")
        {
            // if we have an extension, try direct download
            res.download_res = perform_download(url, title, dest_folder);
        }
        else
        {
            const string& domain = child["data"]["domain"].get_ref<str_cref>();

            if (domain == "v.redd.it")
            {
                // TODO: handle domain v.redd.it
                res.download_res = Download_Res::UNABLE;
            }
            else if (domain == "imgur.com")
            {
                string_cref subreddit = child["data"]["subreddit"].get_ref<str_cref>();
                // TODO: handle domain i.imgur.com
                res.download_res = handle_imgur(subreddit, url,
                                                title, dest_folder);
            }
            else if (domain == "gfycat.com")
            {
                res.download_res = handle_gfycat(url, title, dest_folder);
            }
            else
            {
                // unknown domain
                res.download_res = Download_Res::UNABLE;
            }
        }

        return res;
    }
    catch (const std::exception& e)
    {
        cout << std::format("[ECXEP][download_media()] {} url: {}",
                            e.what(), child["data"]["url"].get_ref<str_cref>())
            << endl;
        return {};
    }

}

// reddit image downloader
int rid(const string& subreddit,
        const string& when,
        const string& dest_folder)
{
    try
    {
        curlpp::Cleanup cleanup;

        string after = Utils::get_after_from_file(dest_folder);

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
                                                Utils::to_str(res.download_res)) << endl;
                        }
                        else
                        {
                            cout << std::format("[{}] {:<{}.{}} -> {} url: '{}'",
                                                res.file_id,
                                                res.title, g_PRINT_MAX_LEN, g_PRINT_MAX_LEN,
                                                Utils::to_str(res.download_res),
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
            Utils::save_after_to_file(dest_folder, after);
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

