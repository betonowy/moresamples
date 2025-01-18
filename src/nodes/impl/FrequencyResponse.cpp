#include "common.hpp"

#include <audio/filters.hpp>
#include <nodes/nodes.hpp>
#include <nodes/type_info.hpp>

#include <Ctx.hpp>

#include <nlohmann/json.hpp>

namespace {
struct FrequencyResponse : public nodes::INode {
    FrequencyResponse() : INode(TYPE_INFO_STR(FrequencyResponse), 400, 400, true) {}

    void process(Ctx &, std::span<types::Float> buf) override {
        if (buf.empty()) {
            return;
        }

        std::fill(buf.begin(), buf.end(), 0);
        buf[0] = 1;
    }

    void ui(Ctx &ctx) override {
        nk_layout_row_dynamic(ctx.nk, 0, 1);
        {
            int value = window_ir.size();
            const auto new_value = nk_propertyi(ctx.nk, "Window size", 64, value, 16384, 1, 1);

            if (value < new_value) {
                window_ir.resize(value * 2);
                makeDirty();
            } else if (value > new_value) {
                window_ir.resize(value / 2);
                makeDirty();
            }
        }

        if (isDirty()) {
            input.getInput(ctx, window_ir);
            window_fr.resize(window_ir.size() / 2 + 1);
            audio::filter::fft(window_ir, window_fr);
            clearDirty();
        }

        types::Float abs_max = 0;

        for (const auto &value : window_ir) {
            abs_max = std::max(abs_max, value);
        }

        if (abs_max == 0 && input.attached() == nullptr) {
            return;
        }

        nk_layout_row_dynamic(ctx.nk, 0, 1);
        nk_label(ctx.nk, "Impulse response", NK_TEXT_CENTERED);
        nk_layout_row_dynamic(ctx.nk, 140, 1);
        nk_chart_begin(ctx.nk, NK_CHART_LINES, window_ir.size(), -abs_max, abs_max);

        for (const auto &value : window_ir) {
            nk_chart_push(ctx.nk, value);
        }

        nk_chart_end(ctx.nk);

        types::Float fr_min = 0.f;
        types::Float fr_max = 0.f;

        for (const auto &value : window_fr) {
            fr_min = std::min(fr_min, value);
            fr_max = std::max(fr_max, value);
        }

        const auto fr_peak_db = common::cvt::valueToDb(fr_max);

        nk_layout_row_dynamic(ctx.nk, 0, 1);
        nk_labelf(ctx.nk, NK_TEXT_CENTERED, "Frequency response, peak == %.2f dB", fr_peak_db);
        nk_layout_row_dynamic(ctx.nk, 140, 1);
        nk_chart_begin(ctx.nk, NK_CHART_LINES, window_fr.size(), fr_min, fr_max);

        for (const auto &value : window_fr) {
            nk_chart_push(ctx.nk, value);
        }

        nk_chart_end(ctx.nk);
    }

    Attachments attachments(Attachments buffer, AttachmentFilter filter) override {
        return implAttachments(buffer, filter, &input, &output); //
    }

    static constexpr auto k_window_size = "window_size";

    void serializeData(nlohmann::json &json) override {
        json[k_window_size] = window_ir.size(); //
    }

    void deserializeData(const nlohmann::json &json) override {
        if (!json.is_object()) {
            return;
        }

        window_ir.resize(json.value<size_t>(k_window_size, 256)); //
    }

    nodes::Attachment input = nodes::Attachment(this, nodes::Attachment::Role::INPUT, "IR");
    nodes::Attachment output = nodes::Attachment(this, nodes::Attachment::Role::OUTPUT, "IS", true);

    std::vector<types::Float> window_ir = std::vector<types::Float>(256);
    std::vector<types::Float> window_fr = std::vector<types::Float>(256);
};
} // namespace

std::unique_ptr<nodes::INode> nodes::frequencyResponse() { return std::make_unique<FrequencyResponse>(); }
