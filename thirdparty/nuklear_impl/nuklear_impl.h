#pragma once

#include "nuklear.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL3/SDL.h>

NK_API struct nk_context *nk_sdl_init(SDL_Window *win, SDL_Renderer *renderer);
NK_API void nk_sdl_font_stash_begin(struct nk_font_atlas **atlas);
NK_API void nk_sdl_font_stash_end(void);
NK_API int nk_sdl_handle_event(SDL_Event *evt);
NK_API void nk_sdl_render(enum nk_anti_aliasing);
NK_API void nk_sdl_shutdown(void);
NK_API void nk_sdl_handle_grab(void);
NK_API int nk_consume_keyboard(struct nk_context *ctx);
NK_API int nk_consume_mouse(struct nk_context *ctx);

void nk_overview(struct nk_context *nk);
void nk_style_conf(struct nk_context *nk);

#ifdef __cplusplus
}
#endif
