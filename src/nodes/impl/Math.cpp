#include <nodes/nodes.hpp>
#include <nodes/type_info.hpp>

#include <Ctx.hpp>

#include <nlohmann/json.hpp>

#include <cmath>
#include <ranges>

namespace {
struct Math : public nodes::INode {
    Math() : nodes::INode(TYPE_INFO_STR(Math), 150, 60) {}

    enum class Type : int { Mul, Add, Sub, Sin, Cos, Sqr, Cub, SqrSat };

    void process(Ctx &ctx, std::span<types::Float> buf) override {
        switch (type) {
        case Type::Mul:
        case Type::Add:
        case Type::Sub:
            processXY(ctx, buf);
            break;
        case Type::Sin:
        case Type::Cos:
        case Type::Sqr:
        case Type::Cub:
        case Type::SqrSat:
            processX(ctx, buf);
            break;
        }
    }

    void processX(Ctx &ctx, std::span<types::Float> buf) {
        const auto out = std::span(buf);
        const auto in_x = std::span(buf);

        if (const auto attached = input_x.attached()) {
            attached->parent()->process(ctx, in_x);
        } else {
            std::fill(in_x.begin(), in_x.end(), 0.f);
        }

        function(in_x, {}, out);
    }

    void processXY(Ctx &ctx, std::span<types::Float> buf) {
        std::vector<types::Float> buf_2(buf.size());

        const auto out = std::span(buf);
        const auto in_x = std::span(buf);
        const auto in_y = std::span(buf_2);

        if (const auto attached = input_x.attached()) {
            attached->parent()->process(ctx, in_x);
        } else {
            std::fill(in_x.begin(), in_x.end(), 0.f);
        }

        if (const auto attached = input_y.attached()) {
            attached->parent()->process(ctx, in_y);
        } else {
            std::fill(in_y.begin(), in_y.end(), 0.f);
        }

        function(in_x, in_y, out);
    }

    void ui(Ctx &ctx) override {
        const char *type_labels[] = {
            "multiply", "add",    "subtract", "sin", //
            "cos",      "square", "cubic",    "2x/(x^2+1)",
        };

        nk_layout_row_dynamic(ctx.nk, 0, 1);

        {
            const auto prev = type;
            nk_combobox(                                       //
                ctx.nk, type_labels, std::size(type_labels),   //
                reinterpret_cast<int *>(&type), 12, {100, 150} //
            );
            makeDirtyIf(prev != type);
        }

        switch (type) {
        case Type::Mul:
        case Type::Add:
        case Type::Sub:
            break;
        case Type::Sin:
        case Type::Cos:
        case Type::Sqr:
        case Type::Cub:
        case Type::SqrSat:
            input_y.detach();
            break;
        }
    }

    Attachments attachments(Attachments buffer, AttachmentFilter filter) override {
        switch (type) {
        case Type::Mul:
        case Type::Add:
        case Type::Sub:
            return implAttachments(buffer, filter, &input_x, &input_y, &output);
        case Type::Sin:
        case Type::Cos:
        case Type::Sqr:
        case Type::Cub:
        case Type::SqrSat:
            return implAttachments(buffer, filter, &input_x, &output);
            break;
        }
    }

    void function(std::span<const types::Float> x, std::span<const types::Float> y, std::span<types::Float> o) const {
        switch (type) {
        case Type::Mul:
            functionImpl(x, y, o, [](types::Float x, types::Float y) { return x * y; });
            break;
        case Type::Add:
            functionImpl(x, y, o, [](types::Float x, types::Float y) { return x + y; });
            break;
        case Type::Sub:
            functionImpl(x, y, o, [](types::Float x, types::Float y) { return x - y; });
            break;
        case Type::Sin:
            functionImpl(x, o, [](types::Float x) { return std::sin(x); });
            break;
        case Type::Cos:
            functionImpl(x, o, [](types::Float x) { return std::cos(x); });
            break;
        case Type::Sqr:
            functionImpl(x, o, [](types::Float x) { return x * x; });
            break;
        case Type::Cub:
            functionImpl(x, o, [](types::Float x) { return x * x * x; });
            break;
        case Type::SqrSat:
            functionImpl(x, o, [](types::Float x) { return 2 * x / (x * x + 1); });
            break;
        }
    }

    void functionImpl(std::span<const types::Float> xs, std::span<const types::Float> ys, std::span<types::Float> os, auto fn) const {
        for (auto [x, y, o] : std::ranges::views::zip(xs, ys, os)) {
            o = fn(x, y);
        }
    }

    void functionImpl(std::span<const types::Float> xs, std::span<types::Float> os, auto fn) const {
        for (auto [x, o] : std::ranges::views::zip(xs, os)) {
            o = fn(x);
        }
    }

    static constexpr auto k_type = "type";

    void serializeData(nlohmann::json &kvl) override {
        kvl[k_type] = type; //
    }

    void deserializeData(const nlohmann::json &kvl) override {
        type = std::clamp(kvl.value<Type>(k_type, Type::Mul), Type::Mul, Type::SqrSat); //
    }

    nodes::Attachment input_x = nodes::Attachment(this, nodes::Attachment::Role::INPUT, "X");
    nodes::Attachment input_y = nodes::Attachment(this, nodes::Attachment::Role::INPUT, "Y");
    nodes::Attachment output = nodes::Attachment(this, nodes::Attachment::Role::OUTPUT, "Out");

    Type type = Type::Mul;
};
} // namespace

std::unique_ptr<nodes::INode> nodes::math() { return std::make_unique<Math>(); }
