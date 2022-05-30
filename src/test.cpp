#include "pch.h"

namespace Test
{

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

}