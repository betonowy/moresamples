#include "common.hpp"

#include <nodes/nodes.hpp>
#include <nodes/type_info.hpp>

#include <Ctx.hpp>

#include <nlohmann/json.hpp>

namespace {
struct HighPassFilter : public nodes::INode {
    HighPassFilter() : INode(TYPE_INFO_STR(HighPassFilter), 200, 200) {}

    void process(Ctx &ctx, std::span<types::Float> buf) override {
        input.getInput(ctx, buf);

        const auto inv_alpha = types::Float(1) - alpha;
        types::Float cap_value = 0;

        for (auto &v : buf) {
            cap_value = cap_value * inv_alpha + v * alpha;
            v = v - cap_value;
        }
    }

    void ui(Ctx &ctx) override {
        nk_layout_row_dynamic(ctx.nk, 0, 1);
        {
            float value = alpha;
            const auto new_value = nk_propertyf(ctx.nk, "alpha", 1e-4, value, 0.999, 1e-3, common::valuePerPx(value));

            if (value != new_value) {
                alpha = new_value;
                makeDirty();
            }
        }
    }

    Attachments attachments(Attachments buffer, AttachmentFilter filter) override {
        return implAttachments(buffer, filter, &input, &output); //
    }

    static constexpr auto k_alpha = "alpha";

    void serializeData(nlohmann::json &json) override {
        json[k_alpha] = alpha; //
    }

    void deserializeData(const nlohmann::json &json) override {
        alpha = json.value<types::Float>(k_alpha, 0.02); //
    }

    nodes::Attachment input = nodes::Attachment(this, nodes::Attachment::Role::INPUT, "In");
    nodes::Attachment output = nodes::Attachment(this, nodes::Attachment::Role::OUTPUT, "Out");

    types::Float alpha = 0.02;
};
} // namespace

std::unique_ptr<nodes::INode> nodes::highPassFilter() { return std::make_unique<HighPassFilter>(); }
