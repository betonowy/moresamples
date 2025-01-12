#include "common.hpp"

#include <nodes/nodes.hpp>
#include <nodes/type_info.hpp>

#include <Ctx.hpp>

#include <nlohmann/json.hpp>

namespace {
struct CombFilter : public nodes::INode {
    CombFilter() : INode(TYPE_INFO_STR(CombFilter), 170, 80) {}

    void process(Ctx &ctx, std::span<types::Float> buf) override {
        input.getInput(ctx, buf);

        auto src = [&buf](int64_t i) -> types::Float {
            if (i >= 0 && i < static_cast<int64_t>(buf.size())) {
                return buf[i];
            }
            return 0;
        };

        const types::Float sample_rate = ctx.audio.getSampleRate();

        for (int64_t bi = 0; bi < static_cast<int64_t>(buf.size()); ++bi) {
            buf[bi] = buf[bi] + src(std::round(bi - fb_delay * sample_rate)) * (types::Float(1) - fb_decay);
        }
    }

    void ui(Ctx &ctx) override {
        nk_layout_row_dynamic(ctx.nk, 0, 1);
        {
            float value = fb_delay;
            const auto new_value = nk_propertyf(ctx.nk, "delay", 1e-4, value, 0.999, 1e-3, common::valuePerPx(value));

            if (value != new_value) {
                fb_delay = new_value;
                makeDirty();
            }
        }
        {
            float value = fb_decay;
            const auto new_value = nk_propertyf(ctx.nk, "decay", 1e-3, value, 1.0, 1e-3, common::valuePerPx(value));

            if (value != new_value) {
                fb_decay = new_value;
                makeDirty();
            }
        }
    }

    Attachments attachments(Attachments buffer, AttachmentFilter filter) override {
        return implAttachments(buffer, filter, &input, &output); //
    }

    static constexpr auto k_fb_decay = "fb_decay";
    static constexpr auto k_fb_delay = "fb_delay";

    void serializeData(nlohmann::json &json) override {
        json[k_fb_decay] = fb_decay;
        json[k_fb_delay] = fb_delay;
    }

    void deserializeData(const nlohmann::json &json) override {
        fb_decay = json.value<types::Float>(k_fb_decay, 0.5);
        fb_delay = json.value<types::Float>(k_fb_delay, 0.01);
    }

    nodes::Attachment input = nodes::Attachment(this, nodes::Attachment::Role::INPUT, "In");
    nodes::Attachment output = nodes::Attachment(this, nodes::Attachment::Role::OUTPUT, "Out");

    types::Float fb_decay = 0.5;
    types::Float fb_delay = 0.01;
};
} // namespace

std::unique_ptr<nodes::INode> nodes::combFilter() { return std::make_unique<CombFilter>(); }
