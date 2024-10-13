#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace gb {

std::vector<uint8_t> load_file(std::filesystem::path path);

}