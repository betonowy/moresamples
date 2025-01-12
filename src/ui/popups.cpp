#include "ui.hpp"

#include <Ctx.hpp>
#include <nuklear.h>

#include <format>

namespace ui::popups {
bool infiniteLoop(Ctx &ctx, std::vector<nodes::INode *> node_stack) {
    bool should_close = false;

    const auto bounds = nk_window_get_bounds(ctx.nk);

    const auto center_x = static_cast<size_t>(ctx.window_size_x / 2) - bounds.x;
    const auto center_y = static_cast<size_t>(ctx.window_size_y / 2) - bounds.y;

    const auto target_h = 500;
    const auto target_w = 300;

    const auto pos_x = center_x - static_cast<size_t>(target_w / 2);
    const auto pos_y = center_y - static_cast<size_t>(target_h / 2);

    if (nk_popup_begin(                                              //
            ctx.nk, NK_POPUP_DYNAMIC, "Infinite Loop",               //
            NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE | NK_WINDOW_BORDER, //
            nk_rect(pos_x, pos_y, target_w, target_h))               //
    ) {
        nk_layout_row_dynamic(ctx.nk, 0, 1);

        for (size_t i = 0; i < node_stack.size(); ++i) {
            const auto text = std::format(                                             //
                "#{}: {}: at pos{{{}, {}}}",                                           //
                i, node_stack[i]->name, node_stack[i]->space.x, node_stack[i]->space.y //
            );
            nk_text(ctx.nk, text.c_str(), text.size(), NK_TEXT_ALIGN_LEFT);
        }
    } else {
        should_close = true;
    }
    nk_popup_end(ctx.nk);

    return should_close;
}
} // namespace ui::popups
