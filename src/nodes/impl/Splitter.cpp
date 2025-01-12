#include <nodes/nodes.hpp>
#include <nodes/type_info.hpp>

#include <Ctx.hpp>

#include <nlohmann/json.hpp>

#include <list>

namespace {
struct Splitter : public nodes::INode {
    Splitter() : INode(TYPE_INFO_STR(Splitter), 180, 120) { pushOutput(); }

    static constexpr size_t COUNT_MIN = 1;
    static constexpr size_t COUNT_MAX = 16;

    // ensure nodes detach before destroying output list
    ~Splitter() {
        input.detach();

        for (auto &output : outputs) {
            output.detach();
        }
    }

    void process(Ctx &ctx, std::span<types::Float> buf) override {
        if (const auto attached = input.attached()) {
            attached->parent()->process(ctx, buf);
        }
    }

    void ui(Ctx &ctx) override {
        nk_layout_row_dynamic(ctx.nk, 0, 1);
        adjustSize(nk_propertyi(ctx.nk, "Count", COUNT_MIN, outputs.size(), COUNT_MAX, 1, 0.2f));
    }

    void pushOutput() {
        outputs.emplace_back(                                                                //
            this, nodes::Attachment::Role::OUTPUT, std::format("Out {}", outputs.size() + 1) //
        );
    }

    void adjustSize(size_t size) {
        while (size < outputs.size()) {
            outputs.pop_back();
        }

        while (size > outputs.size()) {
            pushOutput();
        }
    }

    Attachments attachments(Attachments buffer, AttachmentFilter filter) override {
        std::vector<nodes::Attachment *> list_helper;

        list_helper.reserve(1 + outputs.size());
        list_helper.push_back(&input);

        for (auto &output : outputs) {
            list_helper.push_back(&output);
        }

        return implAttachmentsDynamic(std::move(buffer), filter, list_helper);
    }

    void serializeData(nlohmann::json &kvl) override {
        kvl["size"] = outputs.size(); //
    }

    void deserializeData(const nlohmann::json &kvl) override {
        adjustSize(std::clamp(kvl.value<size_t>("size", 1), COUNT_MIN, COUNT_MAX)); //
    }

    nodes::Attachment input = nodes::Attachment(this, nodes::Attachment::Role::INPUT, "In");
    std::list<nodes::Attachment> outputs;
};
} // namespace

std::unique_ptr<nodes::INode> nodes::splitter() { return std::make_unique<Splitter>(); }
