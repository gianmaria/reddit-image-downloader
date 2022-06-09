#pragma once

auto constexpr g_TITLE_MAX_LEN = 50;
auto constexpr g_PRINT_MAX_LEN = g_TITLE_MAX_LEN;
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

struct Thread_Res
{
    long file_id = -1;
    string title = "null";
    string url = "null";
    Download_Res download_res = Download_Res::INVALID;
};


optional<Response> perform_request(const string& url,
                                   const std::list<string>& headers = {});

optional<string> download_json_from_reddit(
    const string& subreddit,
    const string& when,
    const string& after = "",
    unsigned limit = 100);


Download_Res perform_download(string_cref url,
                              string_cref title,
                              string_cref dest_folder);

Download_Res handle_imgur(string_cref subreddit,
                          string_cref url,
                          string_cref title,
                          string_cref dest_folder);


Download_Res handle_gfycat(
    string_cref url,
    string_cref title,
    string_cref dest_folder);


Thread_Res download_media(long file_id,
                          const njson& child,
                          const string& dest_folder);

// reddit image downloader
int rid(const string& subreddit,
        const string& when,
        const string& dest_folder);
