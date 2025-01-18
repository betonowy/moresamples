#include "common.hpp"

#include <nodes/nodes.hpp>
#include <nodes/type_info.hpp>

#include <Ctx.hpp>
#include <nuklear.h>
#include <ui/ui.hpp>

#include <nlohmann/json.hpp>

#include <chrono>

namespace {
struct AudioOutput : public nodes::INode {
    AudioOutput() : INode(TYPE_INFO_STR(AudioOutput), 280, 250) {}

    void process(Ctx &ctx, std::span<types::Float> buf) override {
        if (const auto attached = input.attached()) {
            attached->parent()->process(ctx, buf);
        }
    }

    void ui(Ctx &ctx) override {
        if (ctx.audio.currentParrent() == nullptr) {
            ctx.audio.setClip({}, this);
        }

        if (ctx.audio.currentParrent() == this) {
            ctx.audio.setVolume(volume);
        }

        nk_layout_row_begin(ctx.nk, NK_DYNAMIC, 20, 3);
        {
            nk_layout_row_push(ctx.nk, 0.10f);

            nk_style_button style = ctx.nk->style.button;
            style.text_normal.b >>= 1;
            style.text_active.b >>= 1;
            style.text_hover.b >>= 1;

            if (!isDirty() || ctx.audio.currentParrent() != this) {
                style.text_normal.r >>= 1;
                style.text_active.r >>= 1;
                style.text_hover.r >>= 1;
            }

            if (nk_button_symbol_styled(ctx.nk, &style, NK_SYMBOL_TRIANGLE_RIGHT)) {
                if (isDirty() || ctx.audio.currentParrent() != this) {
                    if (const auto stack = isChainInfinite(); stack) {
                        popup_infinite_loop_data = stack;
                    } else {
                        std::vector<types::Float> clip(sample_size);

                        const auto t1 = std::chrono::steady_clock::now();
                        process(ctx, clip);
                        const auto t2 = std::chrono::steady_clock::now();

                        us_processing = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

                        ctx.audio.stop();
                        ctx.audio.setClip(clip, this);
                        ctx.audio.setVolume(volume);
                        ctx.audio.play();
                        ctx.audio.ignoreNextFeedbackTimer();

                        clearDirty();
                    }
                } else {
                    ctx.audio.play();
                }
            };
        }
        {
            nk_layout_row_push(ctx.nk, 0.10f);

            nk_style_button style = ctx.nk->style.button;
            style.text_normal.b >>= 1;
            style.text_normal.g >>= 1;
            style.text_active.b >>= 1;
            style.text_active.g >>= 1;
            style.text_hover.b >>= 1;
            style.text_hover.g >>= 1;

            if (nk_button_symbol_styled(ctx.nk, &style, NK_SYMBOL_RECT_SOLID)) {
                ctx.audio.stop();
            }
        }
        {
            nk_layout_row_push(ctx.nk, 0.80f);

            if (isDirty()) {
                nk_label(ctx.nk, "Not processed yet", NK_TEXT_ALIGN_LEFT);
            } else {
                if (us_processing > 1000000) {
                    nk_labelf(ctx.nk, NK_TEXT_ALIGN_LEFT, "Took: %.1f s", us_processing * 1e-6f);
                } else if (us_processing > 1000) {
                    nk_labelf(ctx.nk, NK_TEXT_ALIGN_LEFT, "Took: %.1f ms", us_processing * 1e-3f);
                } else {
                    nk_labelf(ctx.nk, NK_TEXT_ALIGN_LEFT, "Took: %lu us", us_processing);
                }
            }
        }
        nk_layout_row_end(ctx.nk);

        nk_layout_row_dynamic(ctx.nk, 0, 1);
        {
            constexpr auto max_seconds = 30;

            {
                float seconds = static_cast<float>(sample_size) / static_cast<float>(ctx.audio.getSampleRate());
                const auto prev = seconds;

                nk_property_float(ctx.nk, "Length [seconds]", //
                                  0.f, &seconds, max_seconds, //
                                  0.001f, std::max(seconds / 100.f, 0.001f));

                if (prev != seconds) {
                    sample_size = static_cast<size_t>(seconds * static_cast<float>(ctx.audio.getSampleRate()));
                    makeDirty();
                }
            }
            {
                int samples = static_cast<int>(sample_size);
                const auto max = static_cast<int>(ctx.audio.getSampleRate()) * max_seconds;
                const auto prev = samples;

                nk_property_int(ctx.nk, "Length [samples]", 0, &samples, max, 1, 1);

                if (prev != samples) {
                    sample_size = static_cast<size_t>(samples);
                    makeDirty();
                }
            }
            {
                float nk_volume = volume;
                const auto prev = nk_volume;

                nk_property_float(ctx.nk, "volume", 1e-3, &nk_volume, 1.f, 1e-3, common::valuePerPx(volume));

                if (prev != nk_volume) {
                    volume = nk_volume;
                    if (ctx.audio.currentParrent() == this) {
                        ctx.audio.setVolume(volume);
                    }
                }
            }
            { sync_size = nk_propertyi(ctx.nk, "window", 16, sync_size, 2048, 16, 16); }
        }

        std::vector<types::Float> window(sync_size);

        ctx.audio.getSampleFeedback(window, [&ctx, this] -> std::optional<types::Float> {
            if (const auto attached = sync.attached()) {
                std::array<types::Float, 1> value;
                attached->parent()->process(ctx, value);
                return value[0];
            }

            return std::nullopt;
        }());

        nk_layout_row_dynamic(ctx.nk, 80, 1);

        nk_chart_begin(ctx.nk, NK_CHART_LINES, window.size(), -1.0f, 1.0f);
        for (const auto &value : window) {
            nk_chart_push(ctx.nk, value);
        }
        nk_chart_end(ctx.nk);

        if (popup_infinite_loop_data) {
            if (ui::popups::infiniteLoop(ctx, *popup_infinite_loop_data)) {
                popup_infinite_loop_data = {};
            }
        }
    }

    Attachments attachments(Attachments buffer, AttachmentFilter filter) override {
        return implAttachments(buffer, filter, &input, &sync); //
    }

    static constexpr auto k_sample_size = "sample_size";
    static constexpr auto k_sync_size = "sync_size";
    static constexpr auto k_volume = "volume";

    void serializeData(nlohmann::json &json) override {
        json[k_sample_size] = sample_size;
        json[k_volume] = volume;
        json[k_sync_size] = sync_size;
    }

    void deserializeData(const nlohmann::json &json) override {
        sample_size = json.value<size_t>(k_sample_size, 1024);
        volume = json.value<types::Float>(k_volume, 1.f);
        sync_size = json.value<size_t>(k_sync_size, 256);
    }

    nodes::Attachment input = nodes::Attachment(this, nodes::Attachment::Role::INPUT, "In");
    nodes::Attachment sync = nodes::Attachment(this, nodes::Attachment::Role::INPUT, "Osc");

    size_t sample_size = 1024;
    size_t us_processing = 0;
    size_t sync_size = 256;
    types::Float volume = 1.f;

    std::optional<std::vector<nodes::INode *>> popup_infinite_loop_data;
};
} // namespace

std::unique_ptr<nodes::INode> nodes::audioOutput() { return std::make_unique<AudioOutput>(); }
