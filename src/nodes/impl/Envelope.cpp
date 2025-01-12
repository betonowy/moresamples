#include "common.hpp"

#include <nodes/nodes.hpp>
#include <nodes/type_info.hpp>

#include <nlohmann/json.hpp>

#include <Ctx.hpp>

namespace {
struct Envelope : public nodes::INode {
    Envelope() : nodes::INode(TYPE_INFO_STR(Envelope), 200, 300) {}

    static constexpr size_t COUNT_MIN = 1;
    static constexpr size_t COUNT_MAX = 65536;

    struct Point {
        types::Float vol = 0.f;
        types::Float len = 0.f;
    };

    void process(Ctx &ctx, std::span<types::Float> buf) override {
        const auto s_per_step = 1.f / static_cast<types::Float>(ctx.audio.getSampleRate());
        types::Float tp = 0.f;
        size_t i = 0;

        for (auto &s : buf) {
            if (i == points.size() - 1) {
                s = points.back().vol;
                continue;
            }

            const auto pa = points[i + 0];
            const auto pb = points[i + 1];

            const auto ratio = tp / pa.len;
            const auto value = pa.vol * (1.f - ratio) + pb.vol * ratio;
            s = value;

            tp += s_per_step;

            if (tp > pa.len) {
                tp = 0.f;
                i += 1;
            }
        }
    }

    void ui(Ctx &ctx) override {
        nk_layout_row_dynamic(ctx.nk, 0, 1);
        adjustSize(nk_propertyi(ctx.nk, "Count", COUNT_MIN, points.size(), COUNT_MAX, 1, 0.2f));

        size_t i = 0;

        for (auto &point : points) {
            const auto str_vol = std::format("[{}]vol", i);
            const auto str_len = std::format("[{}]len", i);

            const auto prev_vol = point.vol;
            point.vol = nk_propertyf(                                //
                ctx.nk, str_vol.c_str(), 0.f, point.vol, 30.f, 1e-4, //
                common::valuePerPx(point.vol)                        //
            );

            makeDirtyIf(prev_vol != point.vol);

            if (i + 1 == points.size()) {
                continue;
            }

            const auto prev_len = point.len;
            point.len = nk_propertyf(                                //
                ctx.nk, str_len.c_str(), 0.f, point.len, 30.f, 1e-4, //
                common::valuePerPx(point.len)                        //
            );

            makeDirtyIf(prev_len != point.len);

            ++i;
        }
    }

    void adjustSize(size_t size) {
        while (size < points.size()) {
            points.pop_back();
        }

        while (size > points.size()) {
            points.emplace_back();
        }
    }

    Attachments attachments(Attachments buffer, AttachmentFilter filter) override { //
        return implAttachments(buffer, filter, &output);
    }

    static constexpr auto k_points = "points";
    static constexpr auto k_vol = "vol";
    static constexpr auto k_len = "len";

    void serializeData(nlohmann::json &json) override {
        nlohmann::json json_points;

        for (const auto &point : points) {
            json_points.push_back({
                {k_vol, point.vol},
                {k_len, point.len},
            });
        }

        json[k_points] = std::move(json_points);
    }

    void deserializeData(const nlohmann::json &json) override {
        const auto json_points = json.value<nlohmann::json>(k_points, {});

        if (!json_points.is_array()) {
            return;
        }

        points.clear();

        for (const auto &point : json_points) {
            points.emplace_back(Point{
                .vol = point.value(k_vol, 0.f),
                .len = point.value(k_len, 0.f),
            });
        }
    }

    nodes::Attachment output = nodes::Attachment(this, nodes::Attachment::Role::OUTPUT, "Out");

    std::vector<Point> points{
        {.vol = 0.f, .len = 0.01f}, //
        {.vol = 1.f, .len = 0.04f}, //
        {.vol = .2f, .len = 0.2f},  //
        {.vol = 0.f, .len = 0.5f},
    };
};
} // namespace

std::unique_ptr<nodes::INode> nodes::envelope() { return std::make_unique<Envelope>(); }
