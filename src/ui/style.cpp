#include "ui.hpp"

#include <Ctx.hpp>
#include <Utl.hpp>

#include <nuklear.h>

void ui::style::setup(Ctx &ctx) {

    const auto s = &ctx.nk->style;
    s->property.padding = {.x = 2, .y = 2};
    s->text.padding = {.x = 2, .y = 1};
    s->window.menu_padding = {.x = 2, .y = 2};
    s->window.min_row_height_padding = 0;

    s->property.rounding = 4;
    s->window.rounding = 4;
    s->window.popup_border = s->window.border;
    s->window.popup_border_color = s->window.border_color;

    // s->window.border = 2;

    // s->button.border = s->window.border;
    // s->chart.border = s->window.border;
    // s->checkbox.border = s->window.border;
    // s->combo.border = s->window.border;
    // s->contextual_button.border = s->window.border;
    // s->edit.border = s->window.border;
    // s->knob.border = s->window.border;
    // s->menu_button.border = s->window.border;
    // s->option.border = s->window.border;
    // s->progress.border = s->window.border;
    // s->property.border = s->window.border;
    // s->scrollh.border = s->window.border;
    // s->scrollv.border = s->window.border;
    // s->slider.border = s->window.border;
    // s->tab.border = s->window.border;

    // s->window.border_color = {.r = 0x60, .g = 0x60, .b = 0x60, .a = 0xff};

    // s->button.border_color = s->window.border_color;
    // s->chart.border_color = s->window.border_color;
    // s->checkbox.border_color = s->window.border_color;
    // s->combo.border_color = s->window.border_color;
    // s->contextual_button.border_color = s->window.border_color;
    // s->edit.border_color = s->window.border_color;
    // s->knob.border_color = s->window.border_color;
    // s->menu_button.border_color = s->window.border_color;
    // s->option.border_color = s->window.border_color;
    // s->progress.border_color = s->window.border_color;
    // s->property.border_color = s->window.border_color;
    // s->scrollh.border_color = s->window.border_color;
    // s->scrollv.border_color = s->window.border_color;
    // s->slider.border_color = s->window.border_color;
    // s->tab.border_color = s->window.border_color;
}

void ui::style::pushMenuBar(Ctx &ctx) { nk_style_push_float(ctx.nk, &ctx.nk->style.window.rounding, 0); }
void ui::style::popMenuBar(Ctx &ctx) { nk_style_pop_float(ctx.nk); }

void ui::style::pushNodes(Ctx &ctx, float margin) { //
    nk_style_push_vec2(ctx.nk, &ctx.nk->style.window.padding, {.x = margin, .y = 0});

    nk_style_push_vec2(                                           //
        ctx.nk, &ctx.nk->style.window.header.padding,             //
        {.x = margin, .y = ctx.nk->style.window.header.padding.y} //
    );
}

void ui::style::popNodes(Ctx &ctx) {
    nk_style_pop_vec2(ctx.nk);
    nk_style_pop_vec2(ctx.nk);
}
