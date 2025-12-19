#include "log.h"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_error.h>

namespace gb::logging {

void init_sdl_logging(SDL_LogPriority lvl = SDL_LOG_PRIORITY_DEBUG);

}

namespace gb {

void sdl_checked(bool success, logging::SourceLocation sloc = std::source_location::current());

auto* sdl_checkptr(auto* ptr, logging::SourceLocation sloc = std::source_location::current()) {
    if(ptr == nullptr) throw_exc({"SDL error: {}", sloc}, SDL_GetError());
    return ptr;
}

}