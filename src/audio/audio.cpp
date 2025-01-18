#include "audio.hpp"

#include <chrono>
#include <cmath>

namespace audio {
void AudioSystem::getSamples(std::span<types::OutFloat> samples) {
    std::lock_guard lck(m_mtx);

    if (!m_playing) {
        std::ranges::fill(samples, 0.f);
        m_volume = m_target_volume;
        return;
    }

    static constexpr auto volume_change = 1e-3f;

    for (auto &s : samples) {
        m_volume = m_volume * (1.f - volume_change) + m_target_volume * volume_change;

        if (m_clip_cursor < m_clip.size()) {
            s = std::clamp(m_clip[m_clip_cursor++], types::Float(-1), types::Float(1)) * m_volume;
            continue;
        }

        s = 0.f;
        m_playing = false;
    }
}

void AudioSystem::ignoreNextFeedbackTimer() { m_feedback_ignore_next = true; }

void AudioSystem::getSampleFeedback(std::span<types::Float> samples, std::optional<types::Float> sync) {
    std::lock_guard lck(m_mtx);
    std::ranges::fill(samples, 0.f);

    const auto tp = std::chrono::steady_clock::now();
    const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(tp - m_feedback_tp).count();
    m_feedback_tp = tp;

    if (!m_feedback_ignore_next) {
        static constexpr auto adjust_ratio = 1.f / 256.f;
        m_feedback_rate = m_feedback_rate * (1.f - adjust_ratio) + (elapsed_ns * adjust_ratio);
    }

    m_feedback_ignore_next = false;
    const auto samples_per_ns = m_sample_rate * 1e-9f;
    m_feedback_cursor += m_feedback_rate * samples_per_ns;

    if (!m_playing) {
        return;
    }

    const auto cursor = [this, sync] {
        if (sync) {
            const auto len = m_sample_rate / *sync;
            return static_cast<int64_t>(m_feedback_cursor - std::fmod(m_feedback_cursor, len));
        }

        return static_cast<int64_t>(m_feedback_cursor);
    }();

    const auto i_start = cursor - static_cast<int64_t>(samples.size());
    const auto i_end = cursor;

    for (int64_t i = i_start, d = 0; i < i_end; ++i, ++d) {
        if (i < 0 || i >= static_cast<int64_t>(m_clip.size())) {
            continue;
        }

        samples[d] = std::clamp(m_clip[i], types::Float(-1), types::Float(1));
    }
}

void AudioSystem::setVolume(types::Float value) {
    std::lock_guard lck(m_mtx);
    m_target_volume = value;
}

void AudioSystem::setClip(std::vector<types::Float> data, const void *parent) {
    std::lock_guard lck(m_mtx);
    m_clip_parent = parent;
    m_clip = std::move(data);
}

void AudioSystem::play(size_t start) {
    std::lock_guard lck(m_mtx);
    m_clip_cursor = start;
    m_feedback_cursor = 0;
    m_playing = true;
    m_loops = 0;
}

void AudioSystem::stop() {
    std::lock_guard lck(m_mtx);
    m_clip_cursor = 0;
    m_playing = false;
}
} // namespace audio
