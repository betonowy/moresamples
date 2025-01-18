#include "common.hpp"

#include <audio/filters.hpp>
#include <nodes/nodes.hpp>
#include <nodes/type_info.hpp>

#include <Ctx.hpp>

#include <nlohmann/json.hpp>

#include <optional>

namespace {
struct BiQuadFilter : public nodes::INode {
    BiQuadFilter() : INode(TYPE_INFO_STR(BiQuadFilter), 200, 120) {}

    enum class Type : int {
        LowPass,
        HighPass,
        BandPass,
        AllPass,
        Notch,
        Peak,
        LowShelf,
        HighShelf,
    };

    void process(Ctx &ctx, std::span<types::Float> buf) override {
        input.getInput(ctx, buf);

        if (!params.has_value()) {
            calculateParams(ctx);
        }

        auto bqf = audio::BiQuadFilter<types::Float>(*params);

        for (auto &v : buf) {
            v = bqf.process(v);
        }
    }

    void ui(Ctx &ctx) override {
        if (!params.has_value()) {
            calculateParams(ctx);
        }

        const char *type_labels[] = {
            "Low pass", "High pass", "Band pass", "All pass",   //
            "Notch",    "Peak",      "Low-shelf", "High-shelf", //
        };

        nk_layout_row_dynamic(ctx.nk, 0, 1);
        {
            const auto prev = type;
            nk_combobox(                                       //
                ctx.nk, type_labels, std::size(type_labels),   //
                reinterpret_cast<int *>(&type), 12, {100, 150} //
            );

            if (prev != type) {
                makeDirty();
                calculateParams(ctx);
            }
        }
        {
            float value = f0;
            const auto new_value = nk_propertyf(ctx.nk, "f0 (Hz)", 20, value, 20000, 1e-3, common::valuePerPx(value));

            if (value != new_value) {
                f0 = new_value;
                makeDirty();
                calculateParams(ctx);
            }
        }
        {
            float value = q;
            const auto new_value = nk_propertyf(ctx.nk, "Q", 0.1, value, 10, 1e-3, common::valuePerPx(value));

            if (value != new_value) {
                q = new_value;
                makeDirty();
                calculateParams(ctx);
            }
        }
        if (type == Type::Peak || type == Type::LowShelf || type == Type::HighShelf) {
            float value = gain_db;
            const auto new_value = nk_propertyf(ctx.nk, "gain (dB)", -96, value, 48, 1, 0.1);

            if (value != new_value) {
                gain_db = new_value;
                makeDirty();
                calculateParams(ctx);
            }
        }
    }

    void calculateParams(Ctx &ctx) {
        const auto sr = ctx.audio.getSampleRate();

        switch (type) {
        case Type::LowPass:
            params = audio::filter::lowPass<types::Float>(sr, f0, q);
            break;
        case Type::HighPass:
            params = audio::filter::highPass<types::Float>(sr, f0, q);
            break;
        case Type::BandPass:
            params = audio::filter::bandPass<types::Float>(sr, f0, q);
            break;
        case Type::AllPass:
            params = audio::filter::allPass<types::Float>(sr, f0, q);
            break;
        case Type::Notch:
            params = audio::filter::notch<types::Float>(sr, f0, q);
            break;
        case Type::Peak:
            params = audio::filter::peak<types::Float>(sr, f0, q, gain_db);
            break;
        case Type::LowShelf:
            params = audio::filter::lowShelf<types::Float>(sr, f0, q, gain_db);
            break;
        case Type::HighShelf:
            params = audio::filter::highShelf<types::Float>(sr, f0, q, gain_db);
            break;
        }
    }

    Attachments attachments(Attachments buffer, AttachmentFilter filter) override {
        return implAttachments(buffer, filter, &input, &output); //
    }

    static constexpr auto k_type = "type";
    static constexpr auto k_gain_db = "gain_db";
    static constexpr auto k_f0 = "f0";
    static constexpr auto k_q = "q";

    void serializeData(nlohmann::json &json) override {
        json[k_type] = type;
        json[k_gain_db] = gain_db;
        json[k_f0] = f0;
        json[k_q] = q;
    }

    void deserializeData(const nlohmann::json &json) override {
        if (!json.is_object()) {
            return;
        }

        type = json.value<Type>(k_type, Type::LowPass);
        gain_db = json.value<types::Float>(k_gain_db, 0.f);
        f0 = json.value<types::Float>(k_f0, 1000);
        q = json.value<types::Float>(k_q, 1);

        params = std::nullopt;
    }

    nodes::Attachment input = nodes::Attachment(this, nodes::Attachment::Role::INPUT, "In");
    nodes::Attachment output = nodes::Attachment(this, nodes::Attachment::Role::OUTPUT, "Out");

    Type type = Type::LowPass;
    types::Float gain_db = 0.f;
    types::Float f0 = 1000;
    types::Float q = 1;

    std::optional<audio::BiQuadFilter<types::Float>::Params> params;
};
} // namespace

std::unique_ptr<nodes::INode> nodes::biQuadFilter() { return std::make_unique<BiQuadFilter>(); }
