#include "log.h"

#include <SDL2/SDL_log.h>

namespace gb::logging {

void init_sdl_logging(SDL_LogPriority lvl = SDL_LOG_PRIORITY_DEBUG);

}

#define GB_SDL_checked(expr) do if((expr) < 0) throw GB_exc("SDL error: {}", SDL_GetError()); while(false)
// if an SDL func can return a nullptr, give us a ref or log the SDL error
#define GB_SDL_checkptr(expr) [&]{auto* p = expr; if(!p) throw GB_exc("SDL error: {}", SDL_GetError()); return p; }()