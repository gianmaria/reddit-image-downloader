#pragma once

#include <Windows.h>

#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>


#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>

#include <nlohmann/json.hpp>

#include "utils.h"

#include <argparse/argparse.hpp>
