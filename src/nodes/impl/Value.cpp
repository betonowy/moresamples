#include "common.hpp"

#include <nodes/nodes.hpp>
#include <nodes/type_info.hpp>

#include <Ctx.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>

namespace {
struct Value : public nodes::INode {
    Value() : INode(TYPE_INFO_STR(Value), 200, 120) {}

    enum class Type : int { Value, Note };

    void process(Ctx &, std::span<types::Float> buf) override {
        std::fill(buf.begin(), buf.end(), getValue()); //
    }

    types::Float getValue() const {
        switch (type) {
        case Type::Value:
            return value;
        case Type::Note:
            return common::cvt::noteToFrequency(note_c0_rel, harmonic, a4_tuning);
            break;
        }
        return 0.f;
    }

    void ui(Ctx &ctx) override {
        nk_layout_row_dynamic(ctx.nk, 0, 2);
        ui_typeRadioButton(ctx.nk, Type::Value, "Value/dB");
        ui_typeRadioButton(ctx.nk, Type::Note, "Note");

        nk_layout_row_dynamic(ctx.nk, 0, 1);
        switch (type) {
        case Type::Value: {
            const auto prev = value;
            value = nk_propertyf(ctx.nk, "", -99999.f, value, 99999.f, 1e-2f, common::valuePerPx(value));
            makeDirtyIf(prev != value);

            nk_labelf(ctx.nk, NK_TEXT_ALIGN_CENTERED, "dBFS: %.1f", common::cvt::valueToDb(value));
            break;
        }
        case Type::Note: {
            const auto prev_c0 = note_c0_rel;
            note_c0_rel = nk_propertyi(ctx.nk, "note", 0, note_c0_rel, 12 * 12, 1, 1);
            makeDirtyIf(prev_c0 != note_c0_rel);

            const auto prev_harmonic = harmonic;
            harmonic = nk_propertyi(ctx.nk, "harmonic", 1, harmonic, 1000, 1, 1);
            makeDirtyIf(prev_harmonic != harmonic);

            const auto prev_a4_tuning = a4_tuning;
            a4_tuning = nk_propertyf(ctx.nk, "A4 tuning", 400, a4_tuning, 500, .1f, common::valuePerPx(a4_tuning));
            makeDirtyIf(prev_a4_tuning != a4_tuning);
            break;
        }
        }
    }

    void ui_typeRadioButton(nk_context *nk, Type activeType, const char *label) {
        nk_bool active = type == activeType;
        if (nk_radio_label(nk, label, &active)) {
            type = activeType;
        }
    }

    Attachments attachments(Attachments buffer, AttachmentFilter filter) override {
        return implAttachments(buffer, filter, &output); //
    }

    static constexpr auto k_type = "type";
    static constexpr auto k_value = "value";
    static constexpr auto k_a4_tuning = "a4";
    static constexpr auto k_note_c0_rel = "note_c0_rel";
    static constexpr auto k_harmonic = "harmonic";

    void serializeData(nlohmann::json &json) override {
        json[k_type] = type;
        json[k_value] = value;
        json[k_a4_tuning] = a4_tuning;
        json[k_note_c0_rel] = note_c0_rel;
        json[k_harmonic] = harmonic;
    }

    void deserializeData(const nlohmann::json &json) override {
        type = json.value<Type>(k_type, Type::Value);
        value = json.value<types::Float>(k_value, 0.f);
        a4_tuning = json.value<types::Float>(k_a4_tuning, 0.f);
        note_c0_rel = json.value<types::Float>(k_note_c0_rel, 0.f);
        harmonic = json.value<types::Float>(k_harmonic, 0.f);
    }

    nodes::Attachment output = nodes::Attachment(this, nodes::Attachment::Role::OUTPUT, "Out");

    Type type = Type::Value;
    // value/dB mode
    types::Float value = 0.5f;
    // note mode
    types::Float a4_tuning = 440.f;
    int note_c0_rel = 0;
    int harmonic = 1;
};
} // namespace

std::unique_ptr<nodes::INode> nodes::value() { return std::make_unique<Value>(); }
