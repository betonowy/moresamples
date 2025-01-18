#include "serialization.hpp"

#include "nodes.hpp"
#include "type_info.hpp"

#include <nlohmann/json.hpp>

#include <unordered_map>

#include <iostream>

namespace {
using Index = std::uint32_t;
}

namespace nodes::str {
std::string serialize(const std::vector<INode *> &nodes) {
    nlohmann::json out;

    std::unordered_map<const void *, Index> attachment_map;
    {
        Index i = 0;
        for (const auto &node : nodes) {
            for (const auto &att : node->attachments()) {
                attachment_map.insert(std::pair(att, ++i));
            }
        }
    }

    auto attachmentIndex = [&attachment_map](nodes::Attachment *p) -> Index {
        if (const auto it = attachment_map.find(p); it != attachment_map.end()) {
            return it->second;
        }
        return 0;
    };

    for (const auto &node : nodes) {
        nlohmann::json attachments;

        for (const auto &att : node->attachments()) {
            attachments.push_back({
                {"index", attachmentIndex(att)},
                {"attached", attachmentIndex(att->attached())},
            });
        };

        nlohmann::json data;

        node->serializeData(data);

        out.push_back({
            {"type", node->name},
            {"attachments", attachments},
            {"data", data},
            {"pos_x", node->space.x},
            {"pos_y", node->space.y},
        });
    }

    return out.dump();
}

namespace {
std::unique_ptr<INode> nodeFromTypeString(std::string_view str) {
    switch (type_info::destringify(str)) {
    case type_info::SerializedType::Value:
        return value();
    case type_info::SerializedType::Math:
        return math();
    case type_info::SerializedType::Generator:
        return generator();
    case type_info::SerializedType::Envelope:
        return envelope();
    case type_info::SerializedType::AudioOutput:
        return audioOutput();
    case type_info::SerializedType::Splitter:
        return splitter();
    case type_info::SerializedType::CombFilter:
        return combFilter();
    case type_info::SerializedType::BiQuadFilter:
        return biQuadFilter();
    case type_info::SerializedType::FrequencyResponse:
        return frequencyResponse();
    default:
    }

    return {};
}
} // namespace

std::vector<std::unique_ptr<INode>> deserialize(const std::string &string) {
    const auto in = nlohmann::json::parse(string);

    struct SavedAttachment {
        Index node_id;
        Index att_id;
        Index global_id;
        Index attached_id;
    };

    std::vector<SavedAttachment> saved_attachments;

    std::vector<std::unique_ptr<INode>> nodes;
    {
        Index node_i = 0;

        for (const auto &entry : in) {
            if (!entry.is_object()) {
                continue;
            }

            const auto typeStr = entry.value<std::string>("type", "");

            auto node = nodeFromTypeString(typeStr);

            if (node == nullptr) {
                std::cerr << std::format("Unknown node type: {}\n", typeStr);
                ++node_i;
                continue;
            }

            const auto data = entry.value<nlohmann::json>("data", {});

            if (data.is_object()) {
                try {
                    node->deserializeData(data);
                } catch (const std::exception &e) {
                    std::cerr << std::format(                             //
                        "Node: {}:{}, failed deserialize - reason: {}\n", //
                        node_i, node->name, e.what()                      //
                    );
                }
            }

            node->space.x = entry.value<float>("pos_x", 100.f);
            node->space.y = entry.value<float>("pos_y", 100.f);

            const auto attachments = entry.value<nlohmann::json>("attachments", {});

            Index att_i = 0;

            for (const auto &att : attachments) {
                saved_attachments.push_back(SavedAttachment{
                    .node_id = node_i,
                    .att_id = att_i++,
                    .global_id = att.value<Index>("index", 0),
                    .attached_id = att.value<Index>("attached", 0),
                });
            }

            nodes.push_back(std::move(node));
            ++node_i;
        }
    }

    for (const auto &satt : saved_attachments) {
        try {
            if (satt.global_id == 0 || satt.attached_id == 0) {
                continue;
            }

            const auto &src_node = nodes.at(satt.node_id);
            const auto src = src_node->attachments().at(satt.att_id);

            const auto &v = saved_attachments;
            const auto cond = [satt](const SavedAttachment &o) { return o.global_id == satt.attached_id; };

            if (const auto it = std::find_if(v.begin(), v.end(), cond); it != v.end()) {
                try {
                    const auto dst = nodes.at(it->node_id)->attachments().at(it->att_id);
                    src->attach(*dst);
                } catch (...) {
                    std::cerr << std::format("failed to attach: {}:{}\n", src_node->uniqueName(), src->name);
                }
            }
        } catch (...) {
            std::cerr << std::format("failed to attach: {}:{}\n", satt.global_id, satt.attached_id);
        }
    }

    return nodes;
}
} // namespace nodes::str
