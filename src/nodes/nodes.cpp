#include "nodes.hpp"

#include <Utl.hpp>

#include <vector>
#include <format>

namespace nodes {
Attachment::Attachment(INode *parent, Role role, std::string name) //
    : name(std::move(name)), role(role), m_parent(parent)          //
{
    utl::assertNotNull(parent);
}

Attachment::~Attachment() { detach(); }

void Attachment::attach(Attachment &other) {
    if (&other == this || other.m_parent == m_parent || role == other.role) {
        return;
    }

    detach();
    other.detach();
    other.m_attached = this;
    m_attached = &other;
    m_parent->makeDirty();
}

void Attachment::detach() {
    if (m_attached) {
        m_attached->m_parent->makeDirty();
        m_attached->m_attached = nullptr;
        m_parent->makeDirty();
        m_attached = nullptr;
    }
}

INode *Attachment::parent() const { return utl::assertNotNull(m_parent); }

Attachment *Attachment::attached() const { return m_attached; }

void Attachment::getInput(Ctx &ctx, std::span<types::Float> output) const {
    assert(role != Attachment::Role::OUTPUT);

    if (m_attached == nullptr) {
        for (auto &value : output) {
            value = 0.f;
        }

        return;
    }

    m_attached->parent()->process(ctx, output);
}

INode::~INode() = default;

INode::Attachments INode::implAttachmentsDynamic( //
    Attachments output,                           //
    AttachmentFilter filter,                      //
    std::span<Attachment *> aps                   //
) {
    output.clear();
    output.reserve(aps.size());

    for (const auto &ap : aps) {
        if (filter.has_value() && *filter != ap->role) {
            continue;
        }

        output.push_back(ap);
    }

    return output;
}

std::string INode::uniqueName() const { return std::format("{}_{}", name, static_cast<const void *>(this)); }

namespace {
std::optional<std::vector<INode *>> safeForEachInChain(           //
    std::vector<INode *> &stack, Attachment::Role role, auto func //
) {
    assert(!stack.empty());

    const auto top = stack.back();

    func(top);

    if (std::any_of(stack.begin(), stack.end() - 1, [top](const auto &v) { return v == top; })) {
        return stack;
    }

    for (auto &attachment : top->attachments({}, role)) {
        if (const auto attached = attachment->attached()) {
            stack.push_back(attached->parent());

            if (const auto opt_stack = safeForEachInChain(stack, role, func)) {
                return opt_stack;
            }

            stack.pop_back();
        }
    }

    return std::nullopt;
}

std::optional<std::vector<INode *>> safeForEachInChain( //
    INode *node, Attachment::Role role, auto func       //
) {
    std::vector<INode *> stack;
    stack.reserve(16);
    stack.push_back(node);
    return safeForEachInChain(stack, role, func);
}
} // namespace

void INode::makeDirty() {
    safeForEachInChain(this, Attachment::Role::OUTPUT, [](INode *p) { p->m_dirty = true; });
}

void INode::clearDirty() {
    safeForEachInChain(this, Attachment::Role::INPUT, [](INode *p) { p->m_dirty = false; });
}

std::optional<std::vector<INode *>> INode::isChainInfinite() {
    return safeForEachInChain(this, Attachment::Role::INPUT, [](INode *) {});
}
} // namespace nodes
