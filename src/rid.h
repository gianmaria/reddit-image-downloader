#pragma once

auto constexpr g_TITLE_MAX_LEN = 70;
auto constexpr g_PRINT_MAX_LEN = 50;
auto constexpr g_upvote_threshold = 1000;
#ifdef _DEBUG
auto constexpr g_num_threads{ 1 }; // num threads
#else
auto constexpr g_num_threads{ 10 }; // num threads
#endif

struct HTTP_Response
{
    string body = "???";
    long code = -1;
    string content_type = "???";
    string resp_headers = "???";
};

enum class Download_Result : uint8_t
{
    INVALID_ENUM,
    DOWNLOADED,
    FAILED,
    SKIPPED, // file already downloaded
    UNABLE // don't know how to download url
};

struct Thread_Result
{
    long file_id = -1;
    string title = "???";
    string url = "???";
    vector<Download_Result> download_res;
};


optional<HTTP_Response> perform_http_request(
    const string& url,
    const std::list<string>& headers = {});

optional<HTTP_Response> request_headers_only(
    const string& url,
    const std::list<string>& headers = {});

optional<string> download_json_from_reddit(
    const string& subreddit,
    const string& when,
    const string& after = "",
    unsigned limit = 100);


Download_Result download_file_to_disk(
    string_cref url,
    string_cref destination);

std::vector<string> get_url_from_imgur(
    string_cref subreddit,
    string_cref url);

string get_url_from_gfycat(
    string_cref url);

string get_url_from_vreddit(
    const njson& child);

std::vector<string> get_url_from_reddit_gallery(
    const njson& child);

Thread_Result download_media(
    long file_id,
    const njson& child,
    const string& dest_folder);

// reddit image downloader
int rid(const string& subreddit,
        const string& when,
        const string& dest_folder);
