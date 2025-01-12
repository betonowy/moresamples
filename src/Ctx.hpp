#pragma once

#include "nuklear.h"

#include <audio/audio.hpp>
#include <nodes/nodes.hpp>

#include <cstddef>
#include <memory>

struct Ctx {
    bool running = true;

    audio::AudioSystem audio;

    nk_context *nk;
    size_t window_size_x;
    size_t window_size_y;

    bool view_grabbed = false;
    float view_intertia = 0.9;
    float view_speed_x = 0;
    float view_speed_y = 0;

    float grid_x = 0;
    float grid_y = 0;
    float grid_size = 64;

    size_t requested_long_names = 0;

    std::vector<std::unique_ptr<nodes::INode>> nodes{};
    nodes::Attachment *attachment_link_begin = nullptr;

    void removeNode(nodes::INode *);
};
