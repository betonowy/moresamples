#include "ui.hpp"

#include <Ctx.hpp>

#include <cmath>

namespace ui {
void grid(Ctx &ctx, float mouse_diff_x, float mouse_diff_y, bool grabbed, bool stop) {
    if (grabbed) {
        ctx.view_speed_x = mouse_diff_x;
        ctx.view_speed_y = mouse_diff_y;
    } else {
        const auto speed_reduction = std::pow(ctx.view_intertia, ctx.nk->delta_time_seconds * 100);
        ctx.view_speed_x *= speed_reduction;
        ctx.view_speed_y *= speed_reduction;
    }

    if (stop) {
        ctx.view_speed_x = 0;
        ctx.view_speed_y = 0;
    }

    for (const auto &node : ctx.nodes) {
        node->space.x += ctx.view_speed_x;
        node->space.y += ctx.view_speed_y;
    }

    ctx.grid_x = std::fmod(ctx.view_speed_x + ctx.grid_x, ctx.grid_size);
    ctx.grid_y = std::fmod(ctx.view_speed_y + ctx.grid_y, ctx.grid_size);
}
} // namespace ui
