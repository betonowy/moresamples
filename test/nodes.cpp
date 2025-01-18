#include <catch2/catch_test_macros.hpp>

#include <nodes/nodes.hpp>
#include <nodes/serialization.hpp>

using namespace nodes;

SCENARIO("AttachmentPoint") {
    struct impl {
        struct Node : public INode {
            Node(std::string name) : INode(std::move(name), 0, 0) {}

            void process(Ctx &, std::span<float>) override {}
            void ui(Ctx &) override {}

            void serializeData(nlohmann::json &) override {}
            void deserializeData(const nlohmann::json &) override {}

            Attachments attachments(Attachments , AttachmentFilter ) override { return {}; }
        };
    };

    impl::Node na("na"), nb("nb"), nc("nc");

    Attachment a1(&na, Attachment::Role::INPUT, "nap");
    Attachment a2(&na, Attachment::Role::OUTPUT, "nap");
    Attachment b1(&nb, Attachment::Role::OUTPUT, "nbp");
    Attachment c1(&nc, Attachment::Role::INPUT, "ncp");

    WHEN("AP attaches to itself") {
        a1.attach(a1);

        THEN("AP remains detached") { REQUIRE(a1.attached() == nullptr); }
    }

    WHEN("AP attaches to other, but the same parent") {
        a1.attach(a2);

        THEN("APs remains detached") {
            REQUIRE(a1.attached() == nullptr);
            REQUIRE(a2.attached() == nullptr);
        }
    }

    WHEN("AP input attaches to other with the same role") {
        a1.attach(c1);

        THEN("APs remains detached") {
            REQUIRE(a1.attached() == nullptr);
            REQUIRE(c1.attached() == nullptr);
        }
    }

    WHEN("AP input attaches to output") {
        a1.attach(b1);

        THEN("APs are attached") {
            REQUIRE(a1.attached() == &b1);
            REQUIRE(b1.attached() == &a1);
        }

        AND_WHEN("AP are reattached") {
            a1.attach(b1);

            THEN("APs remain the same") {
                REQUIRE(a1.attached() == &b1);
                REQUIRE(b1.attached() == &a1);
            }
        }

        AND_WHEN("Another AP breaks attachment") {
            c1.attach(b1);

            THEN("First connection is broken and new is created") {
                REQUIRE(a1.attached() == nullptr);
                REQUIRE(b1.attached() == &c1);
                REQUIRE(c1.attached() == &b1);
            }
        }

        AND_WHEN("AP is detached") {
            a1.detach();

            THEN("Both APs become detached") {
                REQUIRE(a1.attached() == nullptr);
                REQUIRE(b1.attached() == nullptr);
            }
        }
    }
}

SCENARIO("INode") {
    struct impl {
        struct AudioOutput : public INode {
            AudioOutput() : INode("Audio Output", 0, 0) {}

            void process(Ctx &, std::span<float>) override {}
            void ui(Ctx &) override {}

            Attachments attachments(Attachments buffer, AttachmentFilter filter) override { //
                return implAttachments(buffer, filter, &input);
            }

            void serializeData(nlohmann::json &) override {}
            void deserializeData(const nlohmann::json &) override {}

            Attachment input = Attachment(this, Attachment::Role::INPUT, "In");
        };

        struct Generator : public INode {
            Generator() : INode("Generator", 0, 0) {}

            void process(Ctx &, std::span<float>) override {}
            void ui(Ctx &) override {}

            Attachments attachments(Attachments buffer, AttachmentFilter filter) override { //
                return implAttachments(buffer, filter, &output);
            }

            void serializeData(nlohmann::json &) override {}
            void deserializeData(const nlohmann::json &) override {}

            Attachment output = Attachment(this, Attachment::Role::OUTPUT, "Out");
        };
    };

    impl::Generator generator;
    impl::AudioOutput output;

    INode &i_generator = generator;
    INode &i_output = output;

    THEN("Generator has one output AP") {
        const auto outputs = i_generator.attachments({}, Attachment::Role::OUTPUT);

        CHECK(outputs.size() == 1);
        CHECK(outputs.at(0)->name == "Out");
        CHECK(outputs.at(0)->role == Attachment::Role::OUTPUT);

        const auto inputs = i_generator.attachments({}, Attachment::Role::INPUT);

        CHECK(inputs.size() == 0);

        const auto all = i_generator.attachments();

        CHECK(all.size() == 1);
        CHECK(all.at(0)->name == "Out");
        CHECK(all.at(0)->role == Attachment::Role::OUTPUT);
    }

    THEN("AudioOutput has one input AP") {
        const auto outputs = i_output.attachments({}, Attachment::Role::OUTPUT);

        CHECK(outputs.size() == 0);

        const auto inputs = i_output.attachments({}, Attachment::Role::INPUT);

        CHECK(inputs.size() == 1);
        CHECK(inputs.at(0)->name == "In");
        CHECK(inputs.at(0)->role == Attachment::Role::INPUT);

        const auto all = i_output.attachments();

        CHECK(all.size() == 1);
        CHECK(all.at(0)->name == "In");
        CHECK(all.at(0)->role == Attachment::Role::INPUT);
    }
}
