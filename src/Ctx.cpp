#include "Ctx.hpp"

#include <algorithm>

void Ctx::removeNode(nodes::INode *node) {
    auto pred = [node](const std::unique_ptr<nodes::INode> &uptr) { return uptr.get() == node; };
    nodes.erase(std::remove_if(nodes.begin(), nodes.end(), pred), nodes.end());
}
