#include "pch.h"

#include "test.h"
#include "rid.h"

int main(int argc, char* argv[])
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    std::locale::global(std::locale("en_US.UTF-8")); // set the C/C++ locale

    Test::run_test();

#if FAKE_MAIN_ARGS
    (void)argc;
    (void)argv;

    const string subreddit = "VaporwaveAesthetics"; // "VaporwaveAesthetics";
    const string when = "all"; // "day"; 
    const string dest = "VaporwaveAesthetics\\all"; // "🌟vaporwave-aesthetics🌟";

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
