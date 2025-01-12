#pragma once

#include <types.hpp>

#include <cmath>
#include <numbers>

namespace common {
namespace gen {
inline types::Float sin(types::Float x) {
    return std::sin(x * std::numbers::pi_v<types::Float> * types::Float(2)); //
}

inline types::Float cos(types::Float x) {
    return std::cos(x * std::numbers::pi_v<types::Float> * types::Float(2)); //
}

inline types::Float halfSin(types::Float x) {
    return std::sin(x * std::numbers::pi_v<types::Float>) - types::Float(2 / std::numbers::pi_v<types::Float>); //
}

inline types::Float halfCos(types::Float x) {
    return std::cos(x * std::numbers::pi_v<types::Float>); //
}

inline types::Float sawtooth(types::Float x) {
    return x * types::Float(2) - types::Float(1); //
}

inline types::Float triangle(types::Float x) {
    return types::Float(4) * x - types::Float(2) -           //
           std::abs(types::Float(4) * x - types::Float(1)) + //
           std::abs(types::Float(-4) * x + types::Float(3)); //
}

inline types::Float square(types::Float x) {
    return x > types::Float(.5) ? types::Float(1) : types::Float(-1); //
}
} // namespace gen

namespace cvt {
inline types::Float noteToFrequency(int c0_rel, int harmonic, types::Float a4_tuning) {
    static constexpr int c0_to_a4 = 48 - 3;
    return std::pow(2.f, static_cast<types::Float>(c0_rel - c0_to_a4) * (1.f / 12.f)) * a4_tuning * harmonic;
}

inline types::Float valueToDb(types::Float value) { return 20.f * std::log10(std::abs(value)); }
} // namespace cvt

inline types::Float valuePerPx(types::Float value) { return std::max<types::Float>(1e-6, std::abs(value / 33)); }
} // namespace common
