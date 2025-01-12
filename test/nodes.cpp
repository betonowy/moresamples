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

            void serializeData(nodes::str::KeyValueList &) override {}
            void deserializeData(const nodes::str::KeyValueList &) override {}

            Attachments attachments(Attachments buffer, AttachmentFilter filter) override { return {}; }
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

            void serializeData(nodes::str::KeyValueList &) override {}
            void deserializeData(const nodes::str::KeyValueList &) override {}

            Attachment input = Attachment(this, Attachment::Role::INPUT, "In");
        };

        struct Generator : public INode {
            Generator() : INode("Generator", 0, 0) {}

            void process(Ctx &, std::span<float>) override {}
            void ui(Ctx &) override {}

            Attachments attachments(Attachments buffer, AttachmentFilter filter) override { //
                return implAttachments(buffer, filter, &output);
            }

            void serializeData(nodes::str::KeyValueList &) override {}
            void deserializeData(const nodes::str::KeyValueList &) override {}

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

SCENARIO("KeyValueList") {
    str::KeyValueList kvl;

    GIVEN("one entry with leading and trailing key spaces") {
        kvl.setRaw("  key   value ");

        THEN("valid key is found") {
            CHECK(kvl.get("key").value() == "value ");
            AND_THEN("cannot be parsed as a float") { CHECK(!kvl.get<float>("key").has_value()); }
            AND_THEN("cannot be parsed as an int") { CHECK(!kvl.get<int>("key").has_value()); }
        }

        THEN("invalid key is not found") { CHECK(!kvl.get("k").has_value()); }
    }

    GIVEN("one entry with an int value") {
        kvl.setRaw("key 30");
        THEN("valid key is found and value can be parsed") { CHECK(kvl.get<int>("key").value() == 30); }
    }

    GIVEN("one entry with a double value") {
        kvl.setRaw("key 30.2");
        THEN("valid key is found and value can be parsed") { CHECK(kvl.get<double>("key").value() == 30.2); }
    }

    GIVEN("one entry with no leading nor trailing space") {
        kvl.setRaw("key value");
        THEN("valid key is found") { CHECK(kvl.get("key").value() == "value"); }
        THEN("invalid key is not found") { CHECK(!kvl.get("k").has_value()); }
    }

    GIVEN("entry with empty value") {
        kvl.setRaw("key");
        THEN("key found empty") { CHECK(kvl.get("key").value() == ""); }
    }

    GIVEN("entry with empty value (spaces)") {
        kvl.setRaw("key ");
        THEN("key found empty") { CHECK(kvl.get("key").value() == ""); }
    }

    GIVEN("invalid entry with just spaces") {
        kvl.setRaw(" ");
        THEN("key is not found") { CHECK(!kvl.get("key").has_value()); }
    }

    GIVEN("invalid entry with empty string") {
        kvl.setRaw("");
        THEN("key is not found") { CHECK(!kvl.get("key").has_value()); }
    }

    GIVEN("two entries with the same key") {
        kvl.setRaw("key 1");
        kvl.setRaw("key 2");
        THEN("first one is found") { CHECK(kvl.get<int>("key").value() == 1); }
    }

    GIVEN("entries set with template functions") {
        kvl.set("float", 1.2f);
        kvl.set("short", 3456);
        kvl.set("string", "text");

        THEN("values can be read and parsed") {
            CHECK(kvl.get<float>("float").value() == 1.2f);
            CHECK(kvl.get<short>("short").value() == 3456);
            CHECK(kvl.get("string").value() == "text");
        }
    }

    THEN("keys with spaces are rejected") {
        CHECK_THROWS(kvl.set("k k", "sth"));
        CHECK_THROWS(kvl.set(" k", "sth"));
        CHECK_THROWS(kvl.set("k ", "sth"));
    }

    GIVEN("reducible entry") {
        kvl.setRaw(" D K V");

        THEN("non-reduced entry is read correctly") { CHECK(kvl.get("D").value() == "K V"); }

        AND_WHEN("is reduced") {
            kvl.reduce();
            THEN("reduced entry is read correctly") { CHECK(kvl.get("K").value() == "V"); }
        }
    }
}
