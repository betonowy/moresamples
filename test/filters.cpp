#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <audio/filters.hpp>

#include <ranges>

SCENARIO("DelayGroup") {
    GIVEN("static delay group") {
        static constexpr auto dg_size = 4;
        auto dg = audio::DelayGroup<float, dg_size>{};

        THEN("all values are 0 on initialization") {
            for (auto i = 0; i < dg_size; ++i) {
                CHECK(dg[i] == 0);
            }
        }

        THEN("last pushed variable is under index 0") {
            for (auto i = 0; i < dg_size * 2; ++i) {
                dg.push(i);
                CHECK(dg[0] == i);
            }
        }

        THEN("variables are pushed in order") {
            dg.push(1), dg.push(2), dg.push(3), dg.push(4);

            CHECK(dg[0] == 4);
            CHECK(dg[1] == 3);
            CHECK(dg[2] == 2);
            CHECK(dg[3] == 1);

            AND_THEN("reset fills all registers") {
                dg.reset(5);

                for (const auto &v : dg) {
                    CHECK(v == 5);
                }
            }
        }
    }
}

SCENARIO("simple RecursiveLinearFilters") {
    audio::RecursiveLinearFilter<float, 2> empty_filter({{}, {}});
    audio::RecursiveLinearFilter<float, 2> identity_filter({{}, {1}});

    audio::RecursiveLinearFilter<float, 2> fir_low_pass_filter_1({{}, {0.5, 0.5}});
    audio::RecursiveLinearFilter<float, 2> fir_low_pass_filter_2({{}, {0.75, 0.25}});

    audio::RecursiveLinearFilter<float, 2> iir_positive_feedback_filter({{1, 1}, {1}});
    audio::RecursiveLinearFilter<float, 2> iir_negative_feedback_filter({{1, -1}, {1}});
    audio::RecursiveLinearFilter<float, 2> iir_low_pass_filter({{1, 0.5}, {0.5}});

    GIVEN("impulse responses") {
        std::array<float, 4> input{1, 0, 0, 0};

        GIVEN("empty parameters") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{0, 0, 0, 0})) {
                CHECK(empty_filter.process(i) == e);
            }
        }

        GIVEN("1:1 filter") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{1, 0, 0, 0})) {
                CHECK(identity_filter.process(i) == e);
            }
        }

        GIVEN("fir low pass filter") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{0.5, 0.5, 0, 0})) {
                CHECK(fir_low_pass_filter_1.process(i) == e);
            }
        }

        GIVEN("fir low pass filter #2") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{0.75, 0.25, 0, 0})) {
                CHECK(fir_low_pass_filter_2.process(i) == e);
            }
        }

        GIVEN("positive feedback") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{1, 1, 1, 1})) {
                CHECK(iir_positive_feedback_filter.process(i) == e);
            }
        }

        GIVEN("negative feedback") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{1, -1, 1, -1})) {
                CHECK(iir_negative_feedback_filter.process(i) == e);
            }
        }

        GIVEN("simple low pass filter") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{0.5, 0.25, 0.125, 0.0625})) {
                CHECK(iir_low_pass_filter.process(i) == e);
            }
        }
    }

    GIVEN("DC response") {
        std::array<float, 4> input{1, 1, 1, 1};

        GIVEN("empty parameters") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{0, 0, 0, 0})) {
                CHECK(empty_filter.process(i) == e);
            }
        }

        GIVEN("1:1 filter") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{1, 1, 1, 1})) {
                CHECK(identity_filter.process(i) == e);
            }
        }

        GIVEN("fir low pass filter") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{0.5, 1, 1, 1})) {
                CHECK(fir_low_pass_filter_1.process(i) == e);
            }
        }

        GIVEN("fir low pass filter #2") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{0.75, 1, 1, 1})) {
                CHECK(fir_low_pass_filter_2.process(i) == e);
            }
        }

        GIVEN("positive feedback") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{1, 2, 3, 4})) {
                CHECK(iir_positive_feedback_filter.process(i) == e);
            }
        }

        GIVEN("negative feedback") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{1, 0, 1, 0})) {
                CHECK(iir_negative_feedback_filter.process(i) == e);
            }
        }

        GIVEN("simple low pass filter") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{0.5, 0.75, 0.875, 0.9375})) {
                CHECK(iir_low_pass_filter.process(i) == e);
            }
        }
    }
}

SCENARIO("BiQuadFilter") {
    audio::BiQuadFilter<float> identity_filter({{}, {1, 0, 0}});

    audio::BiQuadFilter<float> iir_low_pass{{
        {1, -9.8e-17, 0.6},
        {0.4, 0.8, 0.4},
    }};

    GIVEN("impulse response") {
        std::array<float, 4> input{1, 0, 0, 0};

        GIVEN("1:1 filter") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{1, 0, 0, 0})) {
                CHECK(identity_filter.process(i) == e);
            }
        }

        GIVEN("IIR low pass filter") {
            for (const auto [i, e] : std::ranges::views::zip(input, std::array<float, 4>{0.4, 0.8, 0.64, 0.48})) {
                CHECK_THAT(iir_low_pass.process(i), Catch::Matchers::WithinAbs(e, 1e-5));
            }
        }
    }
}
