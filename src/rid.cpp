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
    [x] v.redd.it
    [x] gfycat.com
 [] download images inside a gallery! e.g. https://www.reddit.com/gallery/uw2ikr

 */

optional<HTTP_Response> perform_http_request(const string& url,
                                             const std::list<string>& headers)
{
    try
    {
        using namespace curlpp::options;
        using namespace curlpp::infos;

        curlpp::Easy request;

        std::stringstream header_ss;

        auto header_function_callback = [&header_ss]
        (char* buffer, size_t size, size_t nitems) -> size_t
        {
            header_ss << string(buffer, size * nitems);
            return size * nitems;
        };

        request.setOpt<Url>(url);
        request.setOpt<UserAgent>("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/101.0.4951.64 Safari/537.36");
        request.setOpt<FollowLocation>(true);
        request.setOpt<MaxRedirs>(10);
        request.setOpt<Verbose>(false);
        request.setOpt<HttpHeader>(headers);
        request.setOpt<HeaderFunction>(header_function_callback);

        std::stringstream body_ss(std::ios_base::out |
                                  std::ios_base::binary);
        request.setOpt<WriteStream>(&body_ss);

        request.perform();

        auto code = ResponseCode::get(request);
        auto content_type = ContentType::get(request);

        return HTTP_Response{
            .body = body_ss.str(),
            .code = code,
            .content_type = content_type,
            .resp_headers = header_ss.str()
        };
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

optional<HTTP_Response> request_headers_only(const string& url,
                                             const std::list<string>& headers)
{
    try
    {
        using namespace curlpp::options;
        using namespace curlpp::infos;

        curlpp::Easy request;

        std::stringstream header_ss;

        auto header_function_callback = [&header_ss]
        (char* buffer, size_t size, size_t nitems) -> size_t
        {
            header_ss << string(buffer, size * nitems);
            return size * nitems;
        };

        request.setOpt<Url>(url);
        request.setOpt<UserAgent>("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/101.0.4951.64 Safari/537.36");
        request.setOpt<FollowLocation>(true);
        request.setOpt<MaxRedirs>(10);
        request.setOpt<HttpHeader>(headers);
        request.setOpt<HeaderFunction>(header_function_callback);
        request.setOpt<NoBody>(true);

        request.perform();

        auto code = ResponseCode::get(request);
        auto content_type = ContentType::get(request);

        return HTTP_Response{
            .body = "",
            .code = code,
            .content_type = content_type,
            .resp_headers = header_ss.str()
        };
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

Download_Result download_file_to_disk(string_cref url,
                                      string_cref destination)
{
    if (fs::exists(destination))
        return Download_Result::SKIPPED;

    auto resp = perform_http_request(url);

    if ((not resp.has_value()) or
        resp->code != 200)
        return Download_Result::FAILED;

    std::ofstream ofs(destination,
                      std::ofstream::binary |
                      std::ofstream::trunc);

    if (not ofs.is_open())
    {
        cout << std::format("[WARN] Cannot open file for writing <{}>", destination) << endl;
        return Download_Result::FAILED;
    }

    ofs.write(
        resp->body.data(),
        static_cast<std::streamsize>(resp->body.size())
    );
    ofs.close();

    return Download_Result::DOWNLOADED;
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

    auto resp = perform_http_request(reddit_url.str());

    if (!resp.has_value())
        return {};

    if (resp->code != 200)
    {
        cout << std::format("[ERROR] Json request failed for url {} with code: {}",
                            reddit_url.str(), resp->code) << endl;
        return {};
    }

    return resp->body;
}

std::vector<string> get_url_from_imgur(string_cref subreddit,
                                       string_cref orig_url)
{
    // https://apidocs.imgur.com/#10456589-7167-4b5c-acd3-a1e4eb6a95ed
    // api endpoint:
    // https://api.imgur.com/3/gallery/r/{{subreddit}}/{{subredditImageId}}

    try
    {
        // TODO: handle gifv
        string image_id = Utils::extract_image_id_from_url(orig_url);

        string api_endpoint = "https://api.imgur.com/3/gallery/r/" + subreddit + "/" + image_id;

        auto imgur_client_id = Utils::env("IMGUR_CLIENT_ID");

        if (imgur_client_id == "")
        {
            cout << "[WARN] no client id for imgur inside file .env" << endl;
            return {};
        }

        auto resp = perform_http_request(api_endpoint,
                                         { "Authorization: Client-ID " +
                                          imgur_client_id });

        if (not resp.has_value())
            return {};

        if (resp->code != 200)
            return {};

        njson json = njson::parse(resp->body);

        if (not json["success"].get<bool>())
        {
            cout << "[ERROR] json returned from imgur.com contains an error, status: "
                << json["status"] << endl;
            return {};
        }

        std::vector<string> urls;
        urls.reserve(20); // feels like 20 is a good number

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

        return urls;
    }
    catch (const njson::parse_error& e)
    {
        cout << "[ERROR] malformed json response from imgur.com: " << e.what() << endl;
        return {};
    }
}

string get_url_from_gfycat(string_cref orig_url)
{
    // https://developers.gfycat.com/api/?curl#getting-info-for-a-single-gfycat
    // example: https://api.gfycat.com/v1/gfycats/JampackedUnrulyArcherfish

    try
    {
        string image_id = Utils::extract_image_id_from_url(orig_url);

        if (image_id.find_first_of('-') != string::npos)
        {
            // NOTE: does this always holds true?
            image_id = image_id.substr(0, image_id.find_first_of('-'));
        }

        const string api_endpoint = "https://api.gfycat.com/v1/gfycats/" + image_id;

        auto resp = perform_http_request(api_endpoint);

        if (not resp.has_value())
            return {};

        if (resp->code != 200)
            return {};

        njson json;
        json = njson::parse(resp->body);


        if (not json.contains("gfyItem"))
            return {};

        if (json.contains("errorMessage"))
            return {};

        string* actual_url = nullptr;

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
            return {};
        }

        return actual_url ? *actual_url : "";
    }
    catch (const njson::parse_error& e)
    {
        cout << "Error while parsing gfycat response json: " << e.what() << endl;
        return {};
    }
}

string get_url_from_vreddit(const njson& child)
{
    try
    {
        auto& video_url = child["data"]["secure_media"]
            ["reddit_video"]["fallback_url"].get_ref<string_cref>();

        return video_url;
    }
    catch (...)
    {
        // unable to grab the fallback_url, adiossssssss
        return {};
    }
}

Thread_Result download_media(long file_id,
                             const njson& child,
                             const string& dest_folder)
{
    try
    {
        auto title = Utils::remove_invalid_charaters(
            child["data"]["title"].get_ref<str_cref>());

        if (Utils::UTF8_len(title) > g_TITLE_MAX_LEN)
            Utils::resize_string(title, g_TITLE_MAX_LEN);

        auto orig_url = child["data"]["url"].get_ref<string_cref>();


#if 0
        unsigned upvote = child["data"]["ups"].get<unsigned>();
        if (upvote < g_upvote_threshold)
        {
            return {
                .file_id = file_id,
                .title = title,
                .url = orig_url,
                .download_res = Download_Result::SKIPPED,
            };
        }
#endif // 0


        const string& domain = child["data"]["domain"].get_ref<str_cref>();

        vector<string> urls;

        if (domain == "v.redd.it")
        {
            urls.push_back(get_url_from_vreddit(child));
        }
        else if (domain == "imgur.com")
        {
            string_cref subreddit = child["data"]["subreddit"].get_ref<str_cref>();

            urls = get_url_from_imgur(subreddit, orig_url);
        }
        else if (domain == "gfycat.com")
        {
            urls.push_back(get_url_from_gfycat(orig_url));
        }
        else
        {
            if (Utils::get_file_extension_from_url(orig_url) != "")
            {
                urls.push_back(orig_url);
            }
            else
            {
                // unknown domain and no extension, no good
                return {
                    .file_id = file_id,
                    .title = title,
                    .url = orig_url,
                    .download_res = Download_Result::UNABLE,
                };
            }
        }

        Download_Result download_result = Download_Result::INVALID;

        // TODO: how to hande multiple download from a signle url....
        for (const auto& url : urls)
        {
            auto resp = request_headers_only(url);

            if (not resp.has_value())
                return {
                    .file_id = file_id,
                    .title = title,
                    .url = orig_url,
                    // TODO: mhhhhhhhhhhhhhhhhhh
                    .download_res = Download_Result::FAILED
                };

            // TODO: big ass assumption that Utils::split_string return al least 2 values here
            auto extension = Utils::split_string(resp->content_type, "/")[1];

            auto destination = std::format("{}\\{}.{}",
                                           dest_folder, title, extension);
            download_result = download_file_to_disk(url, destination);
        }

        return {
            .file_id = file_id,
            .title = title,
            .url = orig_url,
            // TODO: mhhhhhhhhhhhhhhhhhh
            .download_res = download_result
        };

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
                                                  after);

            if (not resp.has_value())
            {
                cout << "[WARN] Cannot download json from subreddit " << subreddit << endl;
                break;
            }

#ifdef _DEBUG 
            // save json to disk for debugging purposes 
            {
                std::ofstream out(dest_folder + "/!#_" + subreddit
                                  + "_" + when + "_" + after + ".json",
                                  std::ofstream::trunc |
                                  std::ofstream::binary);

                if (out.is_open())
                {
                    out << *resp;
                }
            }
#endif 

            njson json = njson::parse(*resp);

            if (not (json.contains("data") and
                json["data"].contains("children")))
            {
                throw std::runtime_error("Unexpected json content... 😳");
            }

            vector_cref children = json["data"]["children"].get_ref<vector_cref>();

            auto files_to_download = children.size();
            //cout << "[INFO] Downlaoading " << file_to_download << " files" << endl;

            while (files_to_download > 0)
            {
                std::array<std::future<Thread_Result>, g_num_threads> threads;

                // spawn threads for media downloading
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

                // print thread results
                for (auto& thread : threads)
                {
                    if (thread.valid())
                    {
                        auto thread_res = thread.get(); // blocking, calls wait()

                        string short_title = thread_res.title;
                        if (Utils::UTF8_len(short_title) > g_PRINT_MAX_LEN)
                            Utils::resize_string(short_title, g_PRINT_MAX_LEN);

                        if (thread_res.download_res == Download_Result::DOWNLOADED or
                            thread_res.download_res == Download_Result::SKIPPED)
                        {
                            cout << std::format("[{:04}] {:<{}.{}} -> {}",
                                                thread_res.file_id,
                                                short_title, g_PRINT_MAX_LEN, g_PRINT_MAX_LEN,
                                                Utils::to_str(thread_res.download_res)) << endl;
                        }
                        else
                        {
                            cout << std::format("[{:04}] {:<{}.{}} -> {} url: '{}'",
                                                thread_res.file_id,
                                                short_title, g_PRINT_MAX_LEN, g_PRINT_MAX_LEN,
                                                Utils::to_str(thread_res.download_res),
                                                thread_res.url) << endl;
                        }
                    }
                }
            }

            if (json["data"]["after"].is_null())
            {
                // nothing left do do, we downloaded everything
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

