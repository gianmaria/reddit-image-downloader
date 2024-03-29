#pragma once

#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <iostream>
#include <list>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <argparse/argparse.hpp>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>

#define JSON_USE_IMPLICIT_CONVERSIONS 0
#include <nlohmann/json.hpp>

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
using string_cref = const string&;
