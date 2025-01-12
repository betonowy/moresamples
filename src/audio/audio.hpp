#pragma once

#include <types.hpp>

#include <chrono>
#include <mutex>
#include <span>
#include <vector>

namespace audio {
struct AudioSystem {
    AudioSystem(size_t sample_rate, types::Float target_frame_rate) //
        : m_sample_rate(sample_rate), m_feedback_rate(1e9f / target_frame_rate) {}

    enum class LoopingStyle {
        NO_LOOPING,
        FORWARD,
        PING_PONG,
    };

    void getSamples(std::span<types::OutFloat> samples);
    size_t getSampleRate() const { return m_sample_rate; };
    const void *currentParrent() const { return m_clip_parent; }

    void ignoreNextFeedbackTimer();
    void getSampleFeedback(std::span<types::Float> samples, std::optional<types::Float> sync);

    void setVolume(types::Float);
    void setClip(std::vector<types::Float>, const void *parent);
    void play(size_t start = 0);
    void stop();

private:
    std::mutex m_mtx;
    size_t m_sample_rate;

    types::Float m_target_volume = 1.f;
    types::Float m_volume = 1.f;
    std::vector<types::Float> m_clip;
    const void *m_clip_parent = nullptr;
    size_t m_clip_cursor = 0;
    size_t m_loops = 0;
    bool m_playing = false;

    std::chrono::steady_clock::time_point m_feedback_tp = std::chrono::steady_clock::now();
    types::Float m_feedback_cursor = 0;
    types::Float m_feedback_rate;
    bool m_feedback_ignore_next = true;
};
} // namespace audio
