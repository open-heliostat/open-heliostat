#ifndef SOURCE_TARGET_SEQUENCER_H
#define SOURCE_TARGET_SEQUENCER_H

#include <Arduino.h>
#include <algorithm>
#include <array>
#include <initializer_list>
#include "heliostat.h"

// Sequences source/target coordinates over time and applies the reflected pose
// to the heliostat controllers.
class SourceTargetSequencer {
public:
    struct Frame {
        uint32_t tMs;        // time from sequence start in milliseconds
        double sourceAz;     // source azimuth
        double sourceEl;     // source elevation
        double targetAz;     // target azimuth
        double targetEl;     // target elevation
    };

    static constexpr size_t kMaxFrames = 16;

    explicit SourceTargetSequencer(HeliostatController &controller) : controller(controller) {}

    void clear() {
        running = false;
        frameCount = 0;
        duration = 0;
        startMs = 0;
    }

    bool load(const Frame *framesIn, size_t count, bool loop = false) {
        if (count == 0 || count > kMaxFrames) return false;
        frameCount = count;
        looping = loop;
        running = false;
        for (size_t i = 0; i < count; ++i) {
            frames[i] = framesIn[i];
        }
        std::sort(frames.begin(), frames.begin() + frameCount,
                  [](const Frame &a, const Frame &b) { return a.tMs < b.tMs; });
        duration = frames[frameCount - 1].tMs;
        return true;
    }

    bool load(std::initializer_list<Frame> list, bool loop = false) {
        return load(list.begin(), list.size(), loop);
    }

    void setLooping(bool loop) { looping = loop; }

    void start(uint32_t nowMs = millis()) {
        if (frameCount == 0) return;
        startMs = nowMs;
        running = true;
    }

    void stop() { running = false; }

    bool update(uint32_t nowMs = millis()) {
        if (!running || frameCount == 0) return false;

        if (duration == 0) {
            applyFrame(frames[frameCount - 1]);
            running = looping;
            if (looping) startMs = nowMs;
            return true;
        }

        uint32_t elapsed = nowMs - startMs;
        if (looping) {
            elapsed = duration ? (elapsed % duration) : 0;
        } else if (elapsed >= duration) {
            applyFrame(frames[frameCount - 1]);
            running = false;
            return true;
        }

        // Before first frame
        if (elapsed < frames[0].tMs) {
            applyFrame(frames[0]);
            return true;
        }

        size_t idx = 0;
        while ((idx + 1) < frameCount && frames[idx + 1].tMs <= elapsed) {
            ++idx;
        }

        if ((idx + 1) == frameCount) {
            applyFrame(frames[frameCount - 1]);
            if (!looping) running = false;
            return true;
        }

        const Frame &a = frames[idx];
        const Frame &b = frames[idx + 1];
        double span = static_cast<double>(b.tMs - a.tMs);
        double t = span > 0.0 ? static_cast<double>(elapsed - a.tMs) / span : 0.0;
        Frame interp;
        interp.tMs = elapsed;
        interp.sourceAz = lerp(a.sourceAz, b.sourceAz, t);
        interp.sourceEl = lerp(a.sourceEl, b.sourceEl, t);
        interp.targetAz = lerp(a.targetAz, b.targetAz, t);
        interp.targetEl = lerp(a.targetEl, b.targetEl, t);
        applyFrame(interp);
        return true;
    }

    bool isRunning() const { return running; }
    bool isLooping() const { return looping; }
    uint32_t totalDuration() const { return duration; }
    size_t size() const { return frameCount; }
    bool frameAt(size_t idx, Frame &out) const {
        if (idx >= frameCount) return false;
        out = frames[idx];
        return true;
    }

private:
    HeliostatController &controller;
    std::array<Frame, kMaxFrames> frames{};
    size_t frameCount = 0;
    uint32_t duration = 0;
    uint32_t startMs = 0;
    bool running = false;
    bool looping = false;

    static double lerp(double a, double b, double t) { return b * t + a * (1.0 - t); }

    void applyFrame(const Frame &f) {
        SphericalCoordinate source{f.sourceAz, f.sourceEl};
        SphericalCoordinate target{f.targetAz, f.targetEl};
        auto reflected = controller.reflect(source, target);
        controller.setPosition(reflected);
    }
};

#endif
