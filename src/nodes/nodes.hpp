#pragma once

#include <types.hpp>

#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

struct Ctx;

namespace nodes {
struct INode;

namespace str {
struct KeyValueList;
}

struct Attachment {
    enum class Role { INPUT, OUTPUT };

    Attachment(INode *parent, Role, std::string name);
    ~Attachment();

    Attachment(const Attachment &) = delete;
    Attachment &operator=(const Attachment &) = delete;
    Attachment(Attachment &&) = delete;
    Attachment &operator=(Attachment &&) = delete;

    void attach(Attachment &other);
    void detach();

    INode *parent() const;
    Attachment *attached() const;

    void getInput(Ctx &, std::span<types::Float> output) const;

    const std::string name;
    const Role role;

private:
    INode *m_parent;
    Attachment *m_attached = nullptr;
};

struct INode {
    const std::string name;
    const size_t ui_width;
    const size_t ui_height;

    struct {
        float x = 100, y = 100, w = 0, h = 0;
    } space;

    INode(std::string name, size_t w, size_t h) //
        : name(std::move(name)), ui_width(w), ui_height(h) {}

    virtual ~INode();

    INode(const INode &) = delete;
    INode &operator=(const INode &) = delete;
    INode(INode &&) = delete;
    INode &operator=(INode &&) = delete;

    bool isDirty() const { return m_dirty; };

    virtual void process(Ctx &ctx, std::span<types::Float> output) = 0;
    virtual void ui(Ctx &ctx) = 0;

    virtual void serializeData(nlohmann::json &) = 0;
    virtual void deserializeData(const nlohmann::json &) = 0;

    void makeDirty();

    void makeDirtyIf(bool value) {
        if (value) {
            makeDirty();
        }
    }

    /// Check if chain is finite. If not, returns node stack at the moment of failure.
    std::optional<std::vector<INode *>> isChainInfinite();

    std::string uniqueName() const;

    using Attachments = std::vector<Attachment *>;
    using AttachmentFilter = std::optional<Attachment::Role>;

    virtual Attachments attachments(Attachments = {}, AttachmentFilter = {}) = 0;

protected:
    void clearDirty();

    static Attachments implAttachmentsDynamic( //
        Attachments output,                    //
        AttachmentFilter filter,               //
        std::span<Attachment *> aps            //
    );

    template <typename... Ts>
    static Attachments implAttachments( //
        Attachments output,             //
        AttachmentFilter filter,        //
        Ts... aps                       //
    ) {
        std::array<Attachment *, sizeof...(aps)> arr({aps...});
        return implAttachmentsDynamic(output, filter, std::span(arr));
    }

private:
    bool m_dirty = true;
};

std::unique_ptr<INode> audioOutput();
std::unique_ptr<INode> generator();
std::unique_ptr<INode> envelope();
std::unique_ptr<INode> math();
std::unique_ptr<INode> value();
std::unique_ptr<INode> splitter();
std::unique_ptr<INode> combFilter();
std::unique_ptr<INode> highPassFilter();
} // namespace nodes
