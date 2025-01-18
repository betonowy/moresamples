#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

namespace audio {
template <typename Ring> struct RingIterator {
    constexpr RingIterator(const Ring &ring, size_t index) : ring(ring), index(index) {}

    bool operator==(const RingIterator &other) const { return other.index == index; }

    constexpr RingIterator &operator++() {
        index += 1;
        return *this;
    }

    constexpr RingIterator operator++(int) {
        auto pre = *this;
        index += 1;
        return pre;
    }

    constexpr auto operator*() const { return ring[index]; }

private:
    const Ring &ring;
    size_t index;
};

template <typename T, size_t N> struct DelayGroup {
    static_assert(N > 0, "N must be positive");

    constexpr RingIterator<DelayGroup> begin() const { return RingIterator(*this, 0); }
    constexpr RingIterator<DelayGroup> end() const { return RingIterator(*this, size()); }

    constexpr void push(T value) noexcept {
        last = last == registers.size() ? 0 : last;
        registers[last++] = value;
    }

    [[nodiscard]] constexpr const T &operator[](size_t index) const noexcept {
        assert(index < registers.size());
        return registers[(registers.size() + last - index - 1) % registers.size()];
    }

    constexpr size_t size() const noexcept { return registers.size(); }

    constexpr void reset(T value = {}) noexcept { std::fill(registers.begin(), registers.end(), value); }

private:
    std::array<T, N> registers{};
    size_t last = 0;
};

template <typename T, size_t N> struct RecursiveLinearFilter {
    struct Params {
        std::array<T, N> a{};
        std::array<T, N> b{};
    };

    constexpr RecursiveLinearFilter(Params p) : p_a(p.a), p_b(p.b) {}

    constexpr T process(T input) noexcept {
        T value = {};

        for (size_t i = N - 1; i > 0; --i) {
            value += p_b[i] * dg_b[i - 1] - p_a[i] * dg_a[i - 1];
        }

        value += input * p_b[0];

        dg_b.push(input);
        dg_a.push(value);

        return value;
    }

    constexpr void reset(T value = {}) noexcept {
        dg_a.reset(value);
        dg_b.reset(value);
    }

    constexpr void setup(Params p) noexcept {
        p_a = p.a;
        p_b = p.b;
    }

private:
    DelayGroup<T, N - 1> dg_a;
    DelayGroup<T, N - 1> dg_b;
    std::array<T, N> p_a{};
    std::array<T, N> p_b{};
};

template <typename T> using BiQuadFilter = RecursiveLinearFilter<T, 3>;

namespace filter {
namespace details {
inline double aCoeff(double gain) { return std::pow(10., gain / 40.); }

inline double omega0(double sampling_rate, double f_0) { return 2. * std::numbers::pi * f_0 / sampling_rate; }

inline double alpha(double omega0, double q) { return std::sin(omega0) / (2. * q); }
} // namespace details

template <typename T> BiQuadFilter<T>::Params lowPass(double sampling_rate, double f_0, double q) {
    const auto omega_0 = details::omega0(sampling_rate, f_0);
    const auto alpha = details::alpha(omega_0, q);
    const auto cos_omega_0 = std::cos(omega_0);

    const T b_0 = (1. - cos_omega_0) / 2.;
    const T b_1 = 1. - cos_omega_0;
    const T b_2 = (1. - cos_omega_0) / 2.;
    const T a_0 = 1. + alpha;
    const T a_1 = -2. * cos_omega_0;
    const T a_2 = 1. - alpha;

    return {
        .a = {a_0 / a_0, a_1 / a_0, a_2 / a_0},
        .b = {b_0 / a_0, b_1 / a_0, b_2 / a_0},
    };
}

template <typename T> BiQuadFilter<T>::Params highPass(double sampling_rate, double f0, double q) {
    const auto omega_0 = details::omega0(sampling_rate, f0);
    const auto alpha = details::alpha(omega_0, q);
    const auto cos_omega_0 = std::cos(omega_0);

    const T b_0 = (1. + cos_omega_0) / 2.;
    const T b_1 = -1. - cos_omega_0;
    const T b_2 = (1. + cos_omega_0) / 2.;
    const T a_0 = 1. + alpha;
    const T a_1 = -2. * cos_omega_0;
    const T a_2 = 1. - alpha;

    return {
        .a = {a_0 / a_0, a_1 / a_0, a_2 / a_0},
        .b = {b_0 / a_0, b_1 / a_0, b_2 / a_0},
    };
}

template <typename T> BiQuadFilter<T>::Params bandPass(double sampling_rate, double f0, double q) {
    const auto omega_0 = details::omega0(sampling_rate, f0);
    const auto alpha = details::alpha(omega_0, q);
    const auto cos_omega_0 = std::cos(omega_0);

    const T b_0 = alpha;
    const T b_1 = 0;
    const T b_2 = -alpha;
    const T a_0 = 1. + alpha;
    const T a_1 = -2. * cos_omega_0;
    const T a_2 = 1. - alpha;

    return {
        .a = {a_0 / a_0, a_1 / a_0, a_2 / a_0},
        .b = {b_0 / a_0, b_1 / a_0, b_2 / a_0},
    };
}

template <typename T> BiQuadFilter<T>::Params notch(double sampling_rate, double f0, double q) {
    const auto omega_0 = details::omega0(sampling_rate, f0);
    const auto alpha = details::alpha(omega_0, q);
    const auto cos_omega_0 = std::cos(omega_0);

    const T b_0 = 1.;
    const T b_1 = -2. * cos_omega_0;
    const T b_2 = 1.;
    const T a_0 = 1. + alpha;
    const T a_1 = -2. * cos_omega_0;
    const T a_2 = 1. - alpha;

    return {
        .a = {a_0 / a_0, a_1 / a_0, a_2 / a_0},
        .b = {b_0 / a_0, b_1 / a_0, b_2 / a_0},
    };
}

template <typename T> BiQuadFilter<T>::Params allPass(double sampling_rate, double f0, double q) {
    const auto omega_0 = details::omega0(sampling_rate, f0);
    const auto alpha = details::alpha(omega_0, q);
    const auto cos_omega_0 = std::cos(omega_0);

    const T b_0 = 1. - alpha;
    const T b_1 = -2. * cos_omega_0;
    const T b_2 = 1. + alpha;
    const T a_0 = 1. + alpha;
    const T a_1 = -2. * cos_omega_0;
    const T a_2 = 1. - alpha;

    return {
        .a = {a_0 / a_0, a_1 / a_0, a_2 / a_0},
        .b = {b_0 / a_0, b_1 / a_0, b_2 / a_0},
    };
}

template <typename T> BiQuadFilter<T>::Params peak(double sampling_rate, double f0, double q, double gain) {
    const auto a_coeff = details::aCoeff(gain);
    const auto omega_0 = details::omega0(sampling_rate, f0);
    const auto alpha = details::alpha(omega_0, q);
    const auto cos_omega_0 = std::cos(omega_0);

    const T b_0 = 1. + alpha * a_coeff;
    const T b_1 = -2. * cos_omega_0;
    const T b_2 = 1. - alpha * a_coeff;
    const T a_0 = 1. + alpha / a_coeff;
    const T a_1 = -2. * cos_omega_0;
    const T a_2 = 1. - alpha / a_coeff;

    return {
        .a = {a_0 / a_0, a_1 / a_0, a_2 / a_0},
        .b = {b_0 / a_0, b_1 / a_0, b_2 / a_0},
    };
}

template <typename T> BiQuadFilter<T>::Params lowShelf(double sampling_rate, double f0, double q, double gain) {
    const auto a_coeff = details::aCoeff(gain);
    const auto omega_0 = details::omega0(sampling_rate, f0);
    const auto alpha = details::alpha(omega_0, q);
    const auto cos_omega_0 = std::cos(omega_0);
    const auto sqrt_a_alpha = std::sqrt(a_coeff) * alpha;

    const T b_0 = a_coeff * ((a_coeff + 1) - (a_coeff - 1) * cos_omega_0 + 2 * sqrt_a_alpha);
    const T b_1 = 2 * a_coeff * ((a_coeff - 1) - (a_coeff + 1) * cos_omega_0);
    const T b_2 = a_coeff * ((a_coeff + 1) - (a_coeff - 1) * cos_omega_0 - 2 * sqrt_a_alpha);
    const T a_0 = (a_coeff + 1) + (a_coeff - 1) * cos_omega_0 + 2 * sqrt_a_alpha;
    const T a_1 = -2. * ((a_coeff - 1) + (a_coeff + 1) * cos_omega_0);
    const T a_2 = (a_coeff + 1) + (a_coeff - 1) * cos_omega_0 - 2 * sqrt_a_alpha;

    return {
        .a = {a_0 / a_0, a_1 / a_0, a_2 / a_0},
        .b = {b_0 / a_0, b_1 / a_0, b_2 / a_0},
    };
}

template <typename T> BiQuadFilter<T>::Params highShelf(double sampling_rate, double f0, double q, double gain) {
    const auto a_coeff = details::aCoeff(gain);
    const auto omega_0 = details::omega0(sampling_rate, f0);
    const auto alpha = details::alpha(omega_0, q);
    const auto cos_omega_0 = std::cos(omega_0);
    const auto sqrt_a_alpha = std::sqrt(a_coeff) * alpha;

    const T b_0 = a_coeff * ((a_coeff + 1) + (a_coeff - 1) * cos_omega_0 + 2 * sqrt_a_alpha);
    const T b_1 = -2 * a_coeff * ((a_coeff - 1) + (a_coeff + 1) * cos_omega_0);
    const T b_2 = a_coeff * ((a_coeff + 1) + (a_coeff - 1) * cos_omega_0 - 2 * sqrt_a_alpha);
    const T a_0 = (a_coeff + 1) - (a_coeff - 1) * cos_omega_0 + 2 * sqrt_a_alpha;
    const T a_1 = 2. * ((a_coeff - 1) - (a_coeff + 1) * cos_omega_0);
    const T a_2 = (a_coeff + 1) - (a_coeff - 1) * cos_omega_0 - 2 * sqrt_a_alpha;

    return {
        .a = {a_0 / a_0, a_1 / a_0, a_2 / a_0},
        .b = {b_0 / a_0, b_1 / a_0, b_2 / a_0},
    };
}

void fft(std::span<const float> values, std::span<float> output);

inline void fft(std::span<const double> values, std::span<double> output) {
    std::vector<float> v_2, o_2;
    v_2.reserve(values.size());
    o_2.resize(output.size());

    for (auto &v : values) {
        v_2.push_back(v);
    }

    fft(v_2, o_2);
}
} // namespace filter
} // namespace audio
