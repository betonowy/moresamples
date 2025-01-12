#include "ui.hpp"

#include <Ctx.hpp>
#include <Utl.hpp>

#include <nodes/nodes.hpp>

void ui::menuBar(Ctx &ctx) {
    style::pushMenuBar(ctx);
    DEFER(style::popMenuBar(ctx));

    struct nk_rect menu_bar_box = {
        .x = 0,
        .y = 0,
        .w = static_cast<float>(ctx.window_size_x),
        .h = 25,
    };

    nk_begin(ctx.nk, "__MENU_BAR__", menu_bar_box, NK_WINDOW_NO_SCROLLBAR);
    nk_menubar_begin(ctx.nk);

    nk_layout_row_begin(ctx.nk, NK_STATIC, 15, 5);
    nk_layout_row_push(ctx.nk, 40);

    if (nk_menu_begin_label(ctx.nk, "File", NK_TEXT_LEFT, nk_vec2(120, 200))) {
        nk_layout_row_dynamic(ctx.nk, 20, 1);

        if (nk_menu_item_label(ctx.nk, "Quit", NK_TEXT_LEFT)) {
            ctx.running = false;
        }

        nk_menu_end(ctx.nk);
    }

    nk_layout_row_push(ctx.nk, 80);
    if (nk_menu_begin_label(ctx.nk, "Add node", NK_TEXT_LEFT, nk_vec2(120, 200))) {
        nk_layout_row_dynamic(ctx.nk, 20, 1);

        if (nk_menu_item_label(ctx.nk, "Audio output", NK_TEXT_LEFT)) {
            ctx.nodes.push_back(nodes::audioOutput());
        }

        if (nk_menu_item_label(ctx.nk, "Generator", NK_TEXT_LEFT)) {
            ctx.nodes.push_back(nodes::generator());
        }

        if (nk_menu_item_label(ctx.nk, "Envelope", NK_TEXT_LEFT)) {
            ctx.nodes.push_back(nodes::envelope());
        }

        if (nk_menu_item_label(ctx.nk, "Math", NK_TEXT_LEFT)) {
            ctx.nodes.push_back(nodes::math());
        }

        if (nk_menu_item_label(ctx.nk, "Value", NK_TEXT_LEFT)) {
            ctx.nodes.push_back(nodes::value());
        }

        if (nk_menu_item_label(ctx.nk, "Splitter", NK_TEXT_LEFT)) {
            ctx.nodes.push_back(nodes::splitter());
        }

        if (nk_menu_item_label(ctx.nk, "CombFilter", NK_TEXT_LEFT)) {
            ctx.nodes.push_back(nodes::combFilter());
        }

        if (nk_menu_item_label(ctx.nk, "HighPassFilter", NK_TEXT_LEFT)) {
            ctx.nodes.push_back(nodes::highPassFilter());
        }

        nk_menu_end(ctx.nk);
    }

    nk_layout_row_end(ctx.nk);
    nk_menubar_end(ctx.nk);
    nk_end(ctx.nk);
}
