#ifndef SERVOMOTORCLASS
#define SERVOMOTORCLASS
#include <Arduino.h>
#include "dcmotor.h"
#include "encoder.h"
#include <abstractcontroller.h>
#include "teleplot.h"

struct Servo_Driver : public AbstractController
{
    enum Mode { MODE_PID = 0, MODE_AUTOTUNE = 1 };
    Motor_Driver &motor;
    bool plot = false;
    double derivative = 0;
    double P = 0.05;
    double I = 0;
    double D = 0;
    double S = 0;
    double integral = 0;
    double lastError = 0;
    double curGain = 0;
    double lastDerivative = 0;
    double integralLimit = 1.0; // keeps integral from pushing output beyond motor limits
    double derivativeFilter = 0.2; // simple low-pass on derivative term
    uint32_t lastRunUs = 0;
    uint32_t lastLoopWarnMs = 0;
    uint32_t lastProfileWarnMs = 0;
    double lastAngle = 0.0;
    double maxDerivative = 500.0; // deg/s clamp to reduce noise spikes
    double outputRamp = 0.0; // 0.0 = disabled, >0.0 = max change per second (e.g. 1.0 = full scale/s)
    double lastOutput = 0.0;
    Mode mode = MODE_PID;
    double autoTuneAmp = 0.25;
    double autoTuneBand = 1.0;
    double autoTunePeakHigh = -1e9;
    double autoTunePeakLow = 1e9;
    uint8_t autoTuneCrossings = 0;
    double autoTunePeriodAcc = 0.0;
    uint32_t autoTuneLastCrossMs = 0;
    int autoTuneLastSign = 0;
    uint32_t autoTuneStartMs = 0;
    uint32_t autoTuneMaxMs = 15000;
    bool autoTuneDone = false;
    double autoTuneOutput = 0.0;
    double tunedKu = 0.0;
    double tunedTu = 0.0;
    static constexpr double kPi = 3.14159265358979323846;
    uint32_t lastPlotMs = 0;
    uint16_t telemetryIntervalMs = 100; // throttle telemetry to avoid UDP saturation
    uint32_t lastEncoderMs = 0;
    uint16_t encoderTimeoutMs = 200; // ms without encoder data before stopping motor
    const uint16_t loopWarnThresholdMs = 60; // warn if control loop pauses beyond this
    const uint16_t loopWarnCooldownMs = 500; // throttle warnings
    const uint16_t profileWarnCooldownMs = 500; // throttle section profiling logs
    const uint16_t sectionWarnMs = 20; // warn if a section in run() exceeds this
	Servo_Driver(Motor_Driver &mot, Encoder &encoder)
        : AbstractController(encoder), motor{mot} {}
    uint8_t getType() const override { return 2; }
    void run() {
        if (!enabled) {
            // motor.setSpeed(0);
            return;
        }

        uint32_t now = millis();
        uint32_t tStartUs = micros();
        uint32_t prevRunUs = lastRunUs;

        bool gotNew = encoder.update();
        uint32_t tAfterEncoderUs = micros();
        if (gotNew || encoder.hasNewData()) {
            lastEncoderMs = now;
        } else {
            if (lastEncoderMs == 0) lastEncoderMs = now;
            if (now - lastEncoderMs > encoderTimeoutMs) {
                motor.setSpeed(0);
                return;
            }
        }

        double curAngle = getAngle();

        if (mode == MODE_AUTOTUNE) {
            runAutoTune();
            return;
        }

        double dt = prevRunUs == 0 ? 0.0 : (tStartUs - prevRunUs) / 1000000.0;
        double localTarget = getTarget();

        if (hasLimits) {
            double middle = calcMiddle();
            error = mod(localTarget - middle + 180., 360.) - mod(curAngle - middle + 180., 360.);
        }
        else {
            error = angularDistance(localTarget, curAngle);
        }

        if (dt > 0) {
            integral += error * dt;
            double maxIntegral = integralLimit / max(fabs(I), 1e-6);
            integral = min(max(integral, -maxIntegral), maxIntegral);

            double rawDerivative = (error - lastError) / dt;
            double angleDerivative = (curAngle - lastAngle) / dt;
            double blended = 0.5 * rawDerivative - 0.5 * angleDerivative; // derivative on error and measurement
            blended = max(-maxDerivative, min(maxDerivative, blended));
            derivative = derivativeFilter * blended + (1.0 - derivativeFilter) * derivative;
        }

        double pTerm = error * P;
        double iTerm = integral * I;
        double dTerm = derivative * D;
        double result = pTerm + iTerm + dTerm;

        if (outputRamp > 0.0 && dt > 0.0) {
            double maxChange = outputRamp * dt;
            double change = result - lastOutput;
            change = max(-maxChange, min(maxChange, change));
            result = lastOutput + change;
        }

        curGain = min(max(result, -1.0), 1.0);

        if (fabs(error) < tolerance) {
            curGain = 0;
            integral = 0;
            derivative = 0;
        }

        lastOutput = curGain;
        motor.setSpeed(curGain);
        uint32_t tAfterMotorUs = micros();
        lastError = error;
        lastAngle = curAngle;

        if (plot && (now - lastPlotMs >= telemetryIntervalMs)) {
            TELEPLOT_SEND("angle", curAngle);
            TELEPLOT_SEND("target", localTarget);
            TELEPLOT_SEND("error", error);
            TELEPLOT_SEND("p_term", pTerm);
            TELEPLOT_SEND("i_term", iTerm);
            TELEPLOT_SEND("d_term", dTerm);
            TELEPLOT_SEND("output", curGain);
            TELEPLOT_SEND("mode", mode);
            lastPlotMs = now;
        }

        // Basic profiling to find stalls inside run().
        uint32_t totalUs = tAfterMotorUs - tStartUs;
        uint32_t encoderUs = tAfterEncoderUs - tStartUs;
        uint32_t motorUs = tAfterMotorUs - tAfterEncoderUs;

        // Merge loop lag warning with profiling numbers for clarity.
        if (prevRunUs != 0) {
            uint32_t gap = (tStartUs - prevRunUs) / 1000;
            if (gap > loopWarnThresholdMs && (now - lastLoopWarnMs) > loopWarnCooldownMs) {
                Serial.printf("Servo loop lag: %lu ms (mode %d) enc=%lu us motor=%lu us total=%lu us\n",
                              (unsigned long)gap, (int)mode,
                              (unsigned long)encoderUs, (unsigned long)motorUs, (unsigned long)totalUs);
                lastLoopWarnMs = now;
            }
        }

        if (millis() - lastProfileWarnMs > profileWarnCooldownMs) {
            if (encoderUs > sectionWarnMs * 1000 || motorUs > sectionWarnMs * 1000 || totalUs > loopWarnThresholdMs * 1000) {
                Serial.printf("Servo profile: enc=%lu us motor=%lu us total=%lu us\n", (unsigned long)encoderUs, (unsigned long)motorUs, (unsigned long)totalUs);
                lastProfileWarnMs = millis();
            }
        }

        // Update lastRunMs at the end so gap calculations use the previous timestamp.
        lastRunUs = tStartUs;
    }
    void setAngle(double angle) override {
        setTarget(angle);
        calcError();
        if (enabled && fabs(error) > tolerance && encoder.hasNewData()) {
            run();
        }
    }
    void init() override {
        motor.init();
        encoder.init();
        getAngle();
        if (encoder.hasNewData()) {
            xSemaphoreTake(_mutex, portMAX_DELAY);
            targetAngle = getAngle();
            xSemaphoreGive(_mutex);
            run();
        }
    }
    void beginAutoTune(double amp = 0.25, double band = 1.0) {
        autoTuneAmp = min(max(amp, 0.05), 1.0);
        autoTuneBand = max(band, 0.1);
        autoTunePeakHigh = -1e9;
        autoTunePeakLow = 1e9;
        autoTuneCrossings = 0;
        autoTunePeriodAcc = 0.0;
        autoTuneLastCrossMs = 0;
        autoTuneLastSign = 0;
        autoTuneStartMs = millis();
        autoTuneDone = false;
        autoTuneOutput = autoTuneAmp;
        integral = 0;
        derivative = 0;
        lastError = 0;
        lastRunUs = 0;
        xSemaphoreTake(_mutex, portMAX_DELAY);
        targetAngle = getAngle();
        xSemaphoreGive(_mutex);
        mode = MODE_AUTOTUNE;
    }
    void cancelAutoTune() {
        mode = MODE_PID;
        autoTuneDone = false;
        motor.setSpeed(0);
    }
    bool isAutoTuneActive() const { return mode == MODE_AUTOTUNE; }
    bool isAutoTuneDone() const { return autoTuneDone; }
private:
    void runAutoTune() {
        uint32_t now = millis();
        if (now - autoTuneStartMs > autoTuneMaxMs) {
            cancelAutoTune();
            return;
        }

        double curAngle = getAngle();
        double e = angularDistance(getTarget(), curAngle);
        autoTunePeakHigh = max(autoTunePeakHigh, e);
        autoTunePeakLow = min(autoTunePeakLow, e);

        int sign = (e > 0) - (e < 0);
        if (sign != 0 && autoTuneLastSign != 0 && sign != autoTuneLastSign) {
            if (autoTuneLastCrossMs != 0) {
                autoTunePeriodAcc += (now - autoTuneLastCrossMs) / 1000.0;
                autoTuneCrossings++;
            }
            autoTuneLastCrossMs = now;
        }
        if (sign != 0) autoTuneLastSign = sign;

        if (e >= autoTuneBand) autoTuneOutput = autoTuneAmp;
        else if (e <= -autoTuneBand) autoTuneOutput = -autoTuneAmp;

        motor.setSpeed(autoTuneOutput);

        if (autoTuneCrossings >= 6 && autoTunePeakHigh > autoTunePeakLow) {
            double a = (autoTunePeakHigh - autoTunePeakLow) * 0.5;
            double Tu = autoTuneCrossings > 0 ? 2.0 * autoTunePeriodAcc / autoTuneCrossings : 0.0;
            if (a > 1e-6 && Tu > 0.01) {
                tunedKu = 4.0 * autoTuneAmp / (kPi * a);
                tunedTu = Tu;
                P = 0.6 * tunedKu;
                I = 1.0 / (0.5 * tunedTu);
                D = 0.125 * tunedTu;
                mode = MODE_PID;
                autoTuneDone = true;
                integral = 0;
                derivative = 0;
                curGain = 0;
                lastRunUs = 0;
                return;
            }
        }

        if (plot && (now - lastPlotMs >= telemetryIntervalMs)) {
            TELEPLOT_SEND("autotune_error", e);
            TELEPLOT_SEND("autotune_output", autoTuneOutput);
            TELEPLOT_SEND("autotune_crossings", autoTuneCrossings);
            lastPlotMs = now;
        }
    }
};
#endif