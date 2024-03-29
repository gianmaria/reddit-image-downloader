#include "pch.h"

#include "utils.h"
#include "rid.h"

namespace Test
{

void run_test()
{
    assert(Utils::extract_file_extension_from_url("https://i.imgur.com/gBj52nI.jpg") == "jpg");
    assert(Utils::extract_file_extension_from_url("https://i.imgur.com/gBj52nI") == "");
    assert(Utils::extract_file_extension_from_url("https://gfycat.com/MeekWeightyFrogmouth") == "");
    assert(Utils::extract_file_extension_from_url("https://66.media.tumblr.com/8ead6e96ca8e3e8fe16434181e8a1493/tumblr_oruoo12vtR1s5qhggo3_1280.png") == "png");

    assert(Utils::is_extension_allowed(Utils::extract_file_extension_from_url("https://i.redd.it/nqa4sfb8ns191.png")));


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

    {
        assert(Utils::UTF8_len("😀😉😁") == 3);
        assert(Utils::UTF8_len("ЀЄЋЏ") == 4);
        assert(Utils::UTF8_len("ЀЄЋЏ") == 4);
        assert(Utils::UTF8_len("ЍჅდ") == 3);

    }

    {
        string s = "😀😉😁";
        Utils::resize_string(s, 1);
        assert(s == "😀");

        s = "😀😉😁";
        Utils::resize_string(s, 2);
        assert(s == "😀😉");

        s = "😀😉😁";
        Utils::resize_string(s, 3);
        assert(s == "😀😉😁");

        s = "hello ЀЄЋЏ 😀😉😁 ЍჅდ world";
        Utils::resize_string(s, 5);
        assert(s == "hello");

        s = "hello ЀЄЋЏ 😀😉😁 ЍჅდ world";
        Utils::resize_string(s, 9);
        assert(s == "hello ЀЄЋ");

    }

    {
        string url = "https://v.redd.it/r7gh3btvonx31/DASH_720?source=fallback";

        auto response = perform_http_request(url);

        if (not response.has_value())
            return;

        if (response->content_type == "video/mp4")
        {
            std::ofstream ofs("test.mp4",
                              std::ofstream::binary |
                              std::ofstream::trunc);
            const char* data = response->body.data();
            std::streamsize size = 
                static_cast<std::streamsize>(response->body.size());
            ofs.write(data, size);
        }
    }


    {
        string s = "ciao/come/stai//";

        auto res = Utils::split_string(s, "/");

        assert(res.size() == 5);
        assert(res[0] == "ciao");
        assert(res[1] == "come");
        assert(res[2] == "stai");
        assert(res[3] == "");
        assert(res[3] == "");

        s = "a=b";
        res = Utils::split_string(s, "=");
        assert(res.size() == 2);
        assert(res[0] == "a");
        assert(res[1] == "b");

        s = "a?b";
        res = Utils::split_string(s, "=");
        assert(res.size() == 1);
        assert(res[0] == "a?b");

        s = "https://www.foobaz.com/index.html?par1=val1&par2=val2";
        res = Utils::split_string(s, "?");
        assert(res.size() == 2);
        assert(res[0] == "https://www.foobaz.com/index.html");
        assert(res[1] == "par1=val1&par2=val2");

        s = res[1];
        res = Utils::split_string(s, "&");
        assert(res.size() == 2);
        assert(res[0] == "par1=val1");
        assert(res[1] == "par2=val2");

        s = res[1];
        res = Utils::split_string(s, "=");
        assert(res.size() == 2);
        assert(res[0] == "par2");
        assert(res[1] == "val2");


    }
}

}