#include "log.h"

#include <SDL2/SDL_log.h>

namespace gb::log {

void init_sdl_logging(SDL_LogPriority lvl = SDL_LOG_PRIORITY_DEBUG);

}

#define GB_SDL_checked(expr) do if((expr) < 0) throw GB_exc("SDL error: {}", SDL_GetError()); while(false)