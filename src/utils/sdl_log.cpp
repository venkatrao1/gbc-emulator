#include <gb/utils/sdl_log.h>

#include <array>
#include <string_view>

namespace gb::logging {

namespace {

constexpr static std::array<std::string_view, SDL_LOG_PRIORITY_COUNT> PRIO_NAMES{
    "",
    "VERBOSE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "CRITICAL",
};

constexpr std::array<std::string_view, SDL_LOG_CATEGORY_GPU+1> CATEGORY_NAMES{
    "APP",
    "ERROR",
    "ASSERT",
    "SYSTEM",
    "AUDIO",
    "VIDEO",
    "RENDER",
    "INPUT",
    "TEST",
    "GPU",
};

// void* param is userdata - unused
void sdl_log(void*, int category, SDL_LogPriority prio, const char* msg){
    using namespace fg_colors;
    const std::string_view category_str = (category >= 0 && category < CATEGORY_NAMES.size()) ? CATEGORY_NAMES[category] : "OTHER";
    std::cout << GREEN << '[' << PRIO_NAMES[prio] << ']' << MAGENTA << " SDL/" << category_str << ": " << NONE << msg << '\n';
}

}

void init_sdl_logging(SDL_LogPriority lvl) {
    SDL_SetLogPriorities(lvl);
    SDL_SetLogOutputFunction(sdl_log, nullptr);
}

}

namespace gb {

void sdl_checked(bool success, logging::SourceLocation sloc) {
    if(!success) throw_exc({"SDL error: {}", sloc}, SDL_GetError());
}

}