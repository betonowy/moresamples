#include <Ctx.hpp>
#include <audio/audio.hpp>
#include <nodes/serialization.hpp>
#include <ui/ui.hpp>

#include <SDL3/SDL.h>

#include "nuklear.h"
#include "nuklear_impl.h"

#include <filesystem>
#include <fstream>
#include <iostream>

void audioCallback(void *userdata, SDL_AudioStream *stream, int additional_amount, int) {
    auto ctx = reinterpret_cast<Ctx *>(userdata);

    constexpr auto buf_len = 128;

    while (additional_amount > 0) {
        float samples[buf_len];
        ctx->audio.getSamples(samples);
        SDL_PutAudioStreamData(stream, samples, sizeof(samples));
        additional_amount -= buf_len;
    }
}

int main() {
    SDL_Window *win;
    SDL_Renderer *renderer;
    float font_scale = 1;

    struct nk_context *ctx;
    struct nk_colorf bg;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);

    win = SDL_CreateWindow("moresamples", 640, 480, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE);

    if (win == NULL) {
        SDL_Log("Error SDL_CreateWindow %s", SDL_GetError());
        exit(-1);
    }

    renderer = SDL_CreateRenderer(win, NULL);

    SDL_SetRenderVSync(renderer, 1);

    if (renderer == NULL) {
        SDL_Log("Error SDL_CreateRenderer %s", SDL_GetError());
        exit(-1);
    }

    { // scale the renderer output for High-DPI displays
        int render_w, render_h;
        int window_w, window_h;
        float scale_x, scale_y;
        SDL_GetCurrentRenderOutputSize(renderer, &render_w, &render_h);
        SDL_GetWindowSize(win, &window_w, &window_h);
        scale_x = (float)(render_w) / (float)(window_w);
        scale_y = (float)(render_h) / (float)(window_h);
        SDL_SetRenderScale(renderer, scale_x, scale_y);
        font_scale = scale_y;
    }

    ctx = nk_sdl_init(win, renderer);
    {
        struct nk_font_atlas *atlas;
        struct nk_font_config config = nk_font_config(0);
        struct nk_font *font;

        nk_sdl_font_stash_begin(&atlas);
        font = nk_font_atlas_add_default(atlas, 13 * font_scale, &config);
        nk_sdl_font_stash_end();

        font->handle.height /= font_scale;
        nk_style_set_font(ctx, &font->handle);
    }

    float refresh_rate = 60.f;
    {
        int display_count = 0;
        const auto displays = SDL_GetDisplays(&display_count);
        const auto win_display = SDL_GetDisplayForWindow(win);

        if (win_display) {
            const auto mode = SDL_GetDesktopDisplayMode(win_display);

            if (mode) {
                refresh_rate = mode->refresh_rate;
            }
        } else if (display_count > 0) {
            const auto mode = SDL_GetDesktopDisplayMode(displays[0]);

            if (mode) {
                refresh_rate = mode->refresh_rate;
            }
        }
    }

    auto app_ctx = Ctx{
        .audio = audio::AudioSystem(44100, refresh_rate),
        .nk = ctx,
        .window_size_x = 640,
        .window_size_y = 480,
    };

    SDL_AudioSpec audio_spec = {
        .format = SDL_AUDIO_F32,
        .channels = 1,
        .freq = 44100,
    };

    const auto audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec, audioCallback, &app_ctx);

    if (audio_stream == nullptr) {
        SDL_Log("Error SDL_OpenAudioDeviceStream %s", SDL_GetError());
        exit(-1);
    }

    SDL_ResumeAudioStreamDevice(audio_stream);

    ui::style::setup(app_ctx);

    bg.r = 0, bg.g = 0, bg.b = 0, bg.a = 1.0f;

    if (std::filesystem::exists("saved.json")) {
        std::ifstream file("saved.json");

        if (file.good()) {
            using iterator = std::istreambuf_iterator<char>;
            app_ctx.nodes = nodes::str::deserialize(std::string(iterator(file), iterator()));
        }
    }

    SDL_StartTextInput(win);

    bool mouse_grid_grab = false;

    auto tp = std::chrono::steady_clock::now();

    while (app_ctx.running) {
        float mouse_diff_x = 0, mouse_diff_y = 0;
        bool mouse_grid_stop = false;
        {
            const auto tp_next = std::chrono::steady_clock::now();
            ctx->delta_time_seconds = std::chrono::duration_cast<std::chrono::nanoseconds>(tp_next - tp).count() * 1e-9f;
            tp = tp_next;
        }
        {
            nk_input_begin(ctx);

            SDL_Event evt;

            float motion_x = 0, motion_y = 0;

            while (SDL_PollEvent(&evt)) {
                if (evt.type == SDL_EVENT_QUIT) {
                    app_ctx.running = false;
                } else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_MIDDLE) {
                    mouse_grid_grab = true;
                } else if (evt.type == SDL_EVENT_MOUSE_BUTTON_UP && evt.button.button == SDL_BUTTON_MIDDLE) {
                    mouse_grid_grab = false;
                } else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_LEFT) {
                    mouse_grid_stop = true;
                } else if (evt.type == SDL_EVENT_MOUSE_BUTTON_UP && evt.button.button == SDL_BUTTON_LEFT) {
                    mouse_grid_stop = true;
                } else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_RIGHT) {
                    mouse_grid_stop = true;
                } else if (evt.type == SDL_EVENT_MOUSE_BUTTON_UP && evt.button.button == SDL_BUTTON_RIGHT) {
                    mouse_grid_stop = true;
                } else if (evt.type == SDL_EVENT_MOUSE_MOTION) {
                    motion_x += evt.motion.xrel;
                    motion_y += evt.motion.yrel;
                }

                if (!mouse_grid_grab) {
                    nk_sdl_handle_event(&evt);
                }
            }

            if (mouse_grid_grab) {
                evt.button = {
                    .type = SDL_EVENT_MOUSE_BUTTON_UP,
                    .reserved = {},
                    .timestamp = SDL_GetTicks(),
                    .windowID = 0,
                    .which = 0,
                    .button = SDL_BUTTON_LEFT,
                    .down = false,
                    .clicks = 1,
                    .padding = {},
                    .x = -999,
                    .y = -999,
                };

                nk_sdl_handle_event(&evt);

                evt.motion = {
                    .type = SDL_EVENT_MOUSE_MOTION,
                    .reserved = {},
                    .timestamp = SDL_GetTicks(),
                    .windowID = 0,
                    .which = 0,
                    .state = {},
                    .x = 0,
                    .y = 0,
                    .xrel = 0,
                    .yrel = 0,
                };

                nk_sdl_handle_event(&evt);
            }
            nk_sdl_handle_grab();

            if (app_ctx.attachment_link_begin) {
                mouse_grid_grab = false;
            }

            if (mouse_grid_grab) {
                mouse_diff_x = motion_x;
                mouse_diff_y = motion_y;
            }

            nk_input_end(ctx);
        }
        {
            int x, y;
            SDL_GetWindowSize(win, &x, &y);

            app_ctx.window_size_x = static_cast<size_t>(x);
            app_ctx.window_size_y = static_cast<size_t>(y);
        }
        ui::grid(app_ctx, mouse_diff_x, mouse_diff_y, mouse_grid_grab, mouse_grid_stop);
        ui::menuBar(app_ctx);
        ui::nodeEditor(app_ctx);

        SDL_SetRenderDrawColorFloat(renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColorFloat(renderer, 0.1f, 0.2f, 0.4f, 1);

        for (float x = app_ctx.grid_x; x < app_ctx.window_size_x; x += app_ctx.grid_size) {
            SDL_RenderLine(renderer, x, 0, x, app_ctx.window_size_y);
        }

        for (float y = app_ctx.grid_y; y < app_ctx.window_size_y; y += app_ctx.grid_size) {
            SDL_RenderLine(renderer, 0, y, app_ctx.window_size_x, y);
        }

        nk_sdl_render(NK_ANTI_ALIASING_ON);
        SDL_RenderPresent(renderer);
    }

    {
        std::vector<nodes::INode *> p_nodes;

        for (const auto &node : app_ctx.nodes) {
            p_nodes.push_back(node.get());
        }

        std::ofstream("saved.json", std::ios::binary) << nodes::str::serialize(p_nodes) << '\n';
    }

    nk_sdl_shutdown();
    SDL_DestroyAudioStream(audio_stream);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
