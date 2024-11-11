#include <gb/utils/sdl_log.h>

#include <array>
#include <string_view>

namespace gb::logging {

namespace {

constexpr static std::array<std::string_view, SDL_NUM_LOG_PRIORITIES> PRIO_NAMES{
    "",
    "VERBOSE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "CRITICAL",
};

constexpr std::array<std::string_view, SDL_LOG_CATEGORY_RESERVED1> CATEGORY_NAMES{
    "APP",
    "ERROR",
    "ASSERT",
    "SYSTEM",
    "AUDIO",
    "VIDEO",
    "RENDER",
    "INPUT",
    "TEST",
};

// void* param is userdata - unused
void sdl_log(void*, int category, SDL_LogPriority prio, const char* msg){
    using namespace fg_colors;
    const std::string_view category_str = (category >= 0 && category < CATEGORY_NAMES.size()) ? CATEGORY_NAMES[category] : "OTHER";
    std::cout << GREEN << '[' << PRIO_NAMES[prio] << ']' << MAGENTA << " SDL/" << category_str << ": " << NONE << msg << '\n';
}

}

void init_sdl_logging(SDL_LogPriority lvl) {
    SDL_LogSetAllPriority(lvl);
    SDL_LogSetOutputFunction(sdl_log, nullptr);
}

}