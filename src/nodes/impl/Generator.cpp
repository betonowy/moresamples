#include "common.hpp"

#include <nodes/nodes.hpp>
#include <nodes/type_info.hpp>

#include <Ctx.hpp>

#include <nlohmann/json.hpp>

#include <cmath>

namespace {
struct Generator : public nodes::INode {
    Generator() : INode(TYPE_INFO_STR(Generator), 150, 60) {}

    enum class Function : int {
        SIN,
        COS,
        HALFSIN,
        HALFCOS,
        SAWTOOTH,
        TRIANGLE,
        SQUARE,
    };

    void process(Ctx &ctx, std::span<types::Float> buf) override {
        if (const auto attached = input.attached()) {
            attached->parent()->process(ctx, buf);
        } else {
            std::fill(buf.begin(), buf.end(), 0.f);
        }

        const auto inv_sample_rate = 1.f / static_cast<types::Float>(ctx.audio.getSampleRate());
        auto phase = 0.f;

        for (auto &v : buf) {
            const auto rate = v * inv_sample_rate;
            phase += rate;
            phase -= std::floor(phase);
            v = generator(phase);
        }
    }

    void ui(Ctx &ctx) override {
        const char *function_labels[] = {
            "sin",      "cos",      //
            "half sin", "half cos", //
            "sawtooth", "triangle", //
            "square",
        };

        nk_layout_row_dynamic(ctx.nk, 0, 1);

        {
            const auto prev = function;
            nk_combobox(                                             //
                ctx.nk, function_labels, std::size(function_labels), //
                reinterpret_cast<int *>(&function), 12, {100, 150}   //
            );
            makeDirtyIf(prev != function);
        }
    }

    Attachments attachments(Attachments buffer, AttachmentFilter filter) override { //
        return implAttachments(buffer, filter, &input, &output);
    }

    types::Float generator(types::Float phase) const {
        switch (function) {
        case Function::SIN:
            return common::gen::sin(phase);
        case Function::COS:
            return common::gen::cos(phase);
        case Function::HALFSIN:
            return common::gen::halfSin(phase);
        case Function::HALFCOS:
            return common::gen::halfCos(phase);
        case Function::SAWTOOTH:
            return common::gen::sawtooth(phase);
        case Function::TRIANGLE:
            return common::gen::triangle(phase);
        case Function::SQUARE:
            return common::gen::square(phase);
        }
    }

    static constexpr auto k_function = "function";

    void serializeData(nlohmann::json &kvl) override {
        kvl[k_function] = function; //
    }

    void deserializeData(const nlohmann::json &kvl) override {
        function = std::clamp(                              //
            kvl.value<Function>(k_function, Function::SIN), //
            Function::SIN,                                  //
            Function::SQUARE                                //
        );
    }

    nodes::Attachment input = nodes::Attachment(this, nodes::Attachment::Role::INPUT, "Hz");
    nodes::Attachment output = nodes::Attachment(this, nodes::Attachment::Role::OUTPUT, "Out");

    Function function = Function::SIN;
};
} // namespace

std::unique_ptr<nodes::INode> nodes::generator() { return std::make_unique<Generator>(); }
