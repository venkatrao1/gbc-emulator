#include <gb/utils/load_file.h>
#include <gb/utils/log.h>

#include <fstream>
#include <iterator>

namespace gb {

std::vector<uint8_t> load_file(const std::filesystem::path path) {
	std::ifstream infile{path, std::ios::binary};
	if(!infile) {
		throw_exc("Failed to load file \"{}\"", path.string());
	}
	infile.exceptions(std::ios::failbit | std::ios::badbit);
	return {std::istreambuf_iterator{infile}, std::istreambuf_iterator<char>{}};
}

}