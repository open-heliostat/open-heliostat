#ifndef TARGET_SEQUENCER_H
#define TARGET_SEQUENCER_H

#include <Arduino.h>
#include <array>
#include <algorithm>
#include <cmath>
#include "heliostat.h"

// Sequences a single target in the heliostat targetsMap over time (azimuth/elevation).
class TargetSequencer {
public:
    struct Frame {
        uint32_t tMs; // time from sequence start in milliseconds
        double value;
    };

    static constexpr size_t kMaxFrames = 16;

    explicit TargetSequencer(HeliostatController &controller) : controller(controller) {}

    void clear() {
        running = false;
        startMs = 0;
        azCount = 0;
        elCount = 0;
        azDuration = 0;
        elDuration = 0;
    }

    bool load(const Frame *azFramesIn, size_t azFrameCount,
              const Frame *elFramesIn, size_t elFrameCount,
              bool loop = false) {
        if ((azFrameCount == 0 && elFrameCount == 0) || azFrameCount > kMaxFrames || elFrameCount > kMaxFrames) return false;

        azCount = azFrameCount;
        elCount = elFrameCount;
        looping = loop;
        running = false;

        for (size_t i = 0; i < azFrameCount; ++i) azFrames[i] = azFramesIn[i];
        for (size_t i = 0; i < elFrameCount; ++i) elFrames[i] = elFramesIn[i];

        std::sort(azFrames.begin(), azFrames.begin() + azCount, [](const Frame &a, const Frame &b) { return a.tMs < b.tMs; });
        std::sort(elFrames.begin(), elFrames.begin() + elCount, [](const Frame &a, const Frame &b) { return a.tMs < b.tMs; });

        azDuration = azCount ? azFrames[azCount - 1].tMs : 0;
        elDuration = elCount ? elFrames[elCount - 1].tMs : 0;
        return true;
    }

    void setLooping(bool loop) { looping = loop; }
    void setTargetName(const String &name) { targetName = name; }

    void start(uint32_t nowMs = millis()) {
        if (azCount == 0 && elCount == 0) return;
        startMs = nowMs;
        running = true;
    }

    void stop() { running = false; }

    bool update(uint32_t nowMs = millis()) {
        if (!running || (azCount == 0 && elCount == 0)) return false;

        const uint32_t duration = totalDuration();
        if (duration == 0) {
            applyFrame(lastValue(azFrames, azCount), lastValue(elFrames, elCount));
            running = looping;
            if (looping) startMs = nowMs;
            return true;
        }

        uint32_t elapsed = nowMs - startMs;
        if (looping && duration > 0) {
            elapsed = elapsed % duration;
        } else if (elapsed >= duration) {
            applyFrame(sampleValue(azFrames, azCount, azDuration, duration), sampleValue(elFrames, elCount, elDuration, duration));
            running = false;
            return true;
        }

        double azVal = sampleValue(azFrames, azCount, azDuration, elapsed);
        double elVal = sampleValue(elFrames, elCount, elDuration, elapsed);
        applyFrame(azVal, elVal);
        return true;
    }

    bool isRunning() const { return running; }
    bool isLooping() const { return looping; }
    uint32_t totalDuration() const { return azDuration > elDuration ? azDuration : elDuration; }
    size_t sizeAz() const { return azCount; }
    size_t sizeEl() const { return elCount; }
    bool frameAzAt(size_t idx, Frame &out) const { if (idx >= azCount) return false; out = azFrames[idx]; return true; }
    bool frameElAt(size_t idx, Frame &out) const { if (idx >= elCount) return false; out = elFrames[idx]; return true; }
    const String &target() const { return targetName; }

private:
    HeliostatController &controller;
    std::array<Frame, kMaxFrames> azFrames{};
    std::array<Frame, kMaxFrames> elFrames{};
    size_t azCount = 0;
    size_t elCount = 0;
    uint32_t azDuration = 0;
    uint32_t elDuration = 0;
    uint32_t startMs = 0;
    bool running = false;
    bool looping = false;
    String targetName = "Default Target";

    static double lerp(double a, double b, double t) { return b * t + a * (1.0 - t); }

    double sampleValue(const std::array<Frame, kMaxFrames> &frames, size_t count, uint32_t seqDuration, uint32_t elapsed) const {
        if (count == 0) return NAN;
        if (seqDuration == 0) return frames[count - 1].value;

        uint32_t localElapsed = elapsed;
        if (looping && seqDuration > 0) {
            localElapsed = elapsed % seqDuration;
        }

        if (localElapsed <= frames[0].tMs) return frames[0].value;
        size_t idx = 0;
        while ((idx + 1) < count && frames[idx + 1].tMs <= localElapsed) ++idx;
        if ((idx + 1) == count) return frames[count - 1].value;
        const Frame &a = frames[idx];
        const Frame &b = frames[idx + 1];
        double span = static_cast<double>(b.tMs - a.tMs);
        double t = span > 0.0 ? static_cast<double>(localElapsed - a.tMs) / span : 0.0;
        return lerp(a.value, b.value, t);
    }

    double lastValue(const std::array<Frame, kMaxFrames> &frames, size_t count) const {
        return count ? frames[count - 1].value : NAN;
    }

    void applyFrame(double azimuth, double elevation) {
        double useAz = azimuth;
        double useEl = elevation;

        // fill missing axis values from current map entry if one sequence is empty
        auto it = controller.targetsMap.find(targetName);
        if (it != controller.targetsMap.end()) {
            if (isnan(useAz)) useAz = it->second.azimuth;
            if (isnan(useEl)) useEl = it->second.elevation;
        }

        if (isnan(useAz) || isnan(useEl)) return;

        if (!controller.setTarget(targetName, useAz, useEl)) {
            controller.addTarget(targetName);
            controller.setTarget(targetName, useAz, useEl);
        }
    }
};

#endif
