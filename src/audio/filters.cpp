#include <span>

#include <kiss_fftr.h>

#include <cassert>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

namespace {
struct KissFftCtx {
    KissFftCtx(size_t size) : size(size) { cfg = kiss_fftr_alloc(size, 0, nullptr, nullptr); }

    ~KissFftCtx() {
        if (cfg != nullptr) {
            free(cfg);
        }
    }

    KissFftCtx(const KissFftCtx &) = delete;
    KissFftCtx &operator=(const KissFftCtx &) = delete;

    KissFftCtx(KissFftCtx &&o) : size(std::exchange(o.size, 0)), cfg(std::exchange(o.cfg, nullptr)) {}

    KissFftCtx &operator=(KissFftCtx &&o) {
        if (this != &o) {
            std::swap(o.size, size);
            std::swap(o.cfg, cfg);
        }

        return *this;
    }

    size_t size;
    kiss_fftr_cfg cfg;
};
} // namespace

namespace audio::filter {
void fft(std::span<const float> values, std::span<float> output) {
    thread_local KissFftCtx ctx(values.size());

    if (ctx.size != values.size()) {
        ctx = KissFftCtx(values.size());
    }

    std::vector<kiss_fft_cpx> cpx(values.size() / 2 + 1);
    kiss_fftr(ctx.cfg, values.data(), cpx.data());

    assert(output.size() == values.size() / 2 + 1);

    for (size_t i = 0; i < cpx.size(); ++i) {
        output[i] = std::sqrt(cpx[i].i * cpx[i].i + cpx[i].r * cpx[i].r);
    }
}
} // namespace audio::filters
