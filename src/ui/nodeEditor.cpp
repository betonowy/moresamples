#include "ui.hpp"

#include <Ctx.hpp>
#include <Utl.hpp>

namespace {
template <typename T> size_t indexOf(const std::vector<T *> vec, T *ptr) {
    for (size_t i = 0; i < vec.size(); ++i) {
        if (vec[i] == ptr) {
            return i;
        }
    }

    return 0;
}

using Role = nodes::Attachment::Role;

float attachmentSpacing(float height, size_t count) { return height / (count + 1); }

constexpr auto link_width = 2;
constexpr auto dragged_link_width = 3;
constexpr auto circle_radius = 5;
constexpr auto att_labels_margin = 4.f;

struct nk_rect circleRect(size_t index, float spacing, struct nk_rect bounds, Role role) {
    const auto horizontal_offset = role == Role::INPUT ? -circle_radius * 2.f : bounds.w;

    return {
        .x = bounds.x + horizontal_offset,
        .y = bounds.y + spacing * (index + 1) - circle_radius,
        .w = circle_radius * 2,
        .h = circle_radius * 2,
    };
};

struct nk_color circleColor(struct nk_context *nk, struct nk_rect circle) {
    auto color = nk->style.window.border_color;

    if (nk_input_is_mouse_hovering_rect(&nk->input, circle)) {
        color = nk->style.text.color;
    }

    return color;
}

struct nk_vec2 center(struct nk_rect rect) { return {.x = rect.x + rect.w / 2, .y = rect.y + rect.h / 2}; }

bool shouldDetach(struct nk_context *nk, struct nk_rect circle) {
    return nk_input_has_mouse_click_down_in_rect(&nk->input, NK_BUTTON_RIGHT, circle, nk_true); //
}

bool shouldAttach(struct nk_context *nk, struct nk_rect circle) {
    return nk_input_is_mouse_released(&nk->input, NK_BUTTON_LEFT) && //
           nk_input_is_mouse_hovering_rect(&nk->input, circle);
}

struct Curve {
    struct nk_vec2 p0, p1, p2, p3;
    static constexpr auto extension = 100;
};

std::optional<Curve> optDraggingCurve(struct nk_context *nk, struct nk_rect circle, Role role) {
    if (nk_input_has_mouse_click_down_in_rect(&nk->input, NK_BUTTON_LEFT, circle, nk_true)) {
        const auto side = role == Role::INPUT ? +1 : -1;

        struct nk_vec2 i_target = center(circle);
        struct nk_vec2 o_target = nk->input.mouse.pos;
        struct nk_vec2 i_ext = {.x = i_target.x - Curve::extension * side, .y = i_target.y};
        struct nk_vec2 o_ext = {.x = o_target.x + Curve::extension * side, .y = o_target.y};

        return Curve{i_target, i_ext, o_ext, o_target};
    }

    return std::nullopt;
}

void drawCurve(                                     //
    struct nk_command_buffer *canvas,               //
    Curve curve, float width, struct nk_color color //
) {
    nk_stroke_curve(canvas,                                         //
                    curve.p0.x, curve.p0.y, curve.p1.x, curve.p1.y, //
                    curve.p2.x, curve.p2.y, curve.p3.x, curve.p3.y, //
                    width, color);
}

void drawLink(                                                             //
    struct nk_command_buffer *canvas, struct nk_rect bounds,               //
    nodes::INode::Attachments &scratch_buf, nodes::Attachment *attachment, //
    float input_spacing, size_t index, struct nk_color color               //
) {
    const auto attached = attachment->attached();

    if (!attached) {
        return;
    }

    auto &target_points = scratch_buf;
    target_points = attached->parent()->attachments(std::move(target_points), Role::OUTPUT);

    const auto o_space = attached->parent()->space;
    const auto target_spacing = attachmentSpacing(o_space.h, target_points.size());
    const auto target_index = indexOf(target_points, attached);
    struct nk_vec2 i_target = center(circleRect(index, input_spacing, bounds, Role::INPUT));

    const auto o_target = center(circleRect(          //
        target_index, target_spacing,                 //
        {o_space.x, o_space.y, o_space.w, o_space.h}, //
        Role::OUTPUT                                  //
        )                                             //
    );

    struct nk_vec2 o_ext = {.x = o_target.x + Curve::extension, .y = o_target.y};
    struct nk_vec2 i_ext = {.x = i_target.x - Curve::extension, .y = i_target.y};

    drawCurve(canvas, {i_target, i_ext, o_ext, o_target}, link_width, color);
}

void drawAttachmentText(                                                            //
    struct nk_context *nk, struct nk_command_buffer *canvas, struct nk_rect bounds, //
    float spacing, size_t index, Role role, const std::string &name                 //
) {
    const auto text_height = nk->style.font->height;

    const auto text_width = nk->style.font->width( //
        nk->style.font->userdata,                  //
        nk->style.font->height,                    //
        name.c_str(),                              //
        name.size()                                //
    );

    const auto margin = att_labels_margin / 2.f;

    struct nk_rect text_rect = {
        .x = bounds.x + (role == Role::INPUT ? margin : bounds.w - text_width - margin),
        .y = bounds.y + spacing * (index + 1) - text_height / 2,
        .w = text_width,
        .h = text_height,
    };

    nk_draw_text(                                     //
        canvas, text_rect, name.c_str(), name.size(), //
        nk->style.font, {}, nk->style.text.color      //
    );
}
} // namespace

void ui::nodeEditor(Ctx &ctx) {
    struct nk_rect window_box = {
        .x = 0,
        .y = 0,
        .w = static_cast<float>(ctx.window_size_x),
        .h = static_cast<float>(ctx.window_size_y),
    };

    std::vector<nodes::Attachment *> scratch_buf_a;
    std::vector<nodes::Attachment *> scratch_buf_b;
    std::vector<nodes::INode *> nodes_to_remove;

    bool dragging_active = false;

    for (const auto &node : ctx.nodes) {
        struct nk_rect box = {
            .x = node->space.x,
            .y = node->space.y,
            .w = static_cast<float>(node->ui_width),
            .h = static_cast<float>(node->ui_height),
        };

        constexpr auto window_flags =                                   //
            NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE | NK_WINDOW_BORDER | //
            NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE;

        const auto unique_name = node->uniqueName();

        scratch_buf_a = node->attachments(std::move(scratch_buf_a));

        float max_text_width = 0.f;

        for (const auto &attachment : scratch_buf_a) {
            const auto text_width = ctx.nk->style.font->width( //
                ctx.nk->style.font->userdata,                  //
                ctx.nk->style.font->height,                    //
                attachment->name.c_str(),                      //
                attachment->name.size()                        //
            );

            max_text_width = std::max(max_text_width, text_width);
        }

        style::pushNodes(ctx, max_text_width + att_labels_margin);
        DEFER(style::popNodes(ctx));

        if (node->space.w != 0 && node->space.h != 0) {
            nk_window_set_bounds( //
                ctx.nk, unique_name.c_str(),
                {
                    .x = node->space.x,
                    .y = node->space.y,
                    .w = node->space.w,
                    .h = node->space.h,
                } //
            );
        }

        if (nk_begin_titled(ctx.nk, unique_name.c_str(), node->name.c_str(), box, window_flags)) {
            node->ui(ctx);

            auto bounds = nk_window_get_bounds(ctx.nk);

            node->space = {
                .x = bounds.x,
                .y = bounds.y,
                .w = bounds.w,
                .h = bounds.h,
            };

            const auto canvas = nk_window_get_canvas(ctx.nk);
            nk_push_scissor(canvas, window_box);

            auto handleAttachments = [&](Role role) {
                scratch_buf_a = node->attachments(std::move(scratch_buf_a), role);
                const auto spacing = attachmentSpacing(bounds.h, scratch_buf_a.size());

                for (size_t i = 0; i < scratch_buf_a.size(); ++i) {
                    const auto &attachment = scratch_buf_a[i];

                    struct nk_rect c = circleRect(i, spacing, bounds, role);
                    nk_fill_circle(canvas, c, circleColor(ctx.nk, c));

                    drawAttachmentText(ctx.nk, canvas, bounds, spacing, i, role, attachment->name);

                    if (!dragging_active) {
                        if (const auto curve = optDraggingCurve(ctx.nk, c, role)) {
                            drawCurve(canvas, *curve, dragged_link_width, ctx.nk->style.window.border_color);
                            dragging_active = true;
                            ctx.attachment_link_begin = attachment;
                        }
                    }

                    if (shouldDetach(ctx.nk, c)) {
                        attachment->detach();
                    }

                    if (ctx.attachment_link_begin && shouldAttach(ctx.nk, c)) {
                        attachment->attach(*ctx.attachment_link_begin);
                        ctx.attachment_link_begin = nullptr;
                    }

                    if (role != Role::INPUT) {
                        continue;
                    }

                    drawLink(                                         //
                        canvas, bounds, scratch_buf_b, attachment,    //
                        spacing, i, ctx.nk->style.window.border_color //
                    );
                }
            };

            handleAttachments(Role::INPUT);
            handleAttachments(Role::OUTPUT);
        } else {
            nodes_to_remove.push_back(node.get());
        }
        nk_end(ctx.nk);
    }

    if (dragging_active == false) {
        ctx.attachment_link_begin = nullptr;
    }

    for (const auto &node : nodes_to_remove) {
        ctx.removeNode(node);
    }

    if (!nodes_to_remove.empty() && !ctx.nodes.empty()) {
        nk_window_set_focus(ctx.nk, ctx.nodes.back()->uniqueName().c_str());
    }
}
