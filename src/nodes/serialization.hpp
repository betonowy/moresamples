#pragma once

#include <memory>
#include <string>
#include <vector>

namespace nodes {
struct INode;
}

namespace nodes::str {
std::string serialize(const std::vector<INode *> &);
std::vector<std::unique_ptr<INode>> deserialize(const std::string &);
} // namespace nodes::str
