#ifndef MOVEMENTSEQUENCER_H
#define MOVEMENTSEQUENCER_H

#include <Arduino.h>
#include <algorithm>
#include <array>
#include <initializer_list>
#include "abstractcontroller.h"

// Basic linear keyframe sequencer that drives any AbstractController target.
class MovementSequencer {
public:
    struct Keyframe {
        uint32_t tMs; // time from sequence start in milliseconds
        double value; // target value (angle or other AbstractController target units)
    };

    static constexpr size_t kMaxFrames = 16;

    explicit MovementSequencer(AbstractController &controller) : controller(controller) {}

    void clear() {
        running = false;
        frameCount = 0;
        duration = 0;
        startMs = 0;
    }

    bool load(const Keyframe *framesIn, size_t count, bool loop = false) {
        if (count == 0 || count > kMaxFrames) return false;
        frameCount = count;
        looping = loop;
        running = false;
        for (size_t i = 0; i < count; ++i) {
            frames[i] = framesIn[i];
        }
        std::sort(frames.begin(), frames.begin() + frameCount,
                  [](const Keyframe &a, const Keyframe &b) { return a.tMs < b.tMs; });
        duration = frames[frameCount - 1].tMs;
        return true;
    }

    bool load(std::initializer_list<Keyframe> list, bool loop = false) {
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
            controller.setTarget(frames[frameCount - 1].value);
            controller.calcError();
            running = looping;
            if (looping) startMs = nowMs;
            return true;
        }

        uint32_t elapsed = nowMs - startMs;
        if (looping) {
            elapsed = elapsed % duration;
        } else if (elapsed >= duration) {
            controller.setTarget(frames[frameCount - 1].value);
            controller.calcError();
            running = false;
            return true;
        }

        // Handle case where elapsed time is before the first keyframe
        if (elapsed < frames[0].tMs) {
            controller.setTarget(frames[0].value);
            controller.calcError();
            return true;
        }

        size_t idx = 0;
        while ((idx + 1) < frameCount && frames[idx + 1].tMs <= elapsed) {
            ++idx;
        }

        if ((idx + 1) == frameCount) {
            controller.setTarget(frames[frameCount - 1].value);
            controller.calcError();
            if (!looping) running = false;
            return true;
        }

        const Keyframe &a = frames[idx];
        const Keyframe &b = frames[idx + 1];
        double span = static_cast<double>(b.tMs - a.tMs);
        double t = span > 0.0 ? static_cast<double>(elapsed - a.tMs) / span : 0.0;
        double value = controller.lerp(a.value, b.value, t);
        controller.setTarget(value);
        controller.calcError();
        return true;
    }

    bool isRunning() const { return running; }
    bool isLooping() const { return looping; }
    uint32_t totalDuration() const { return duration; }
    size_t size() const { return frameCount; }
    bool frameAt(size_t idx, Keyframe &out) const {
        if (idx >= frameCount) return false;
        out = frames[idx];
        return true;
    }

private:
    AbstractController &controller;
    std::array<Keyframe, kMaxFrames> frames{};
    size_t frameCount = 0;
    uint32_t duration = 0;
    uint32_t startMs = 0;
    bool running = false;
    bool looping = false;
};

#endif