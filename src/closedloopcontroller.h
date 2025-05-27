#ifndef CLOSED_LOOP_CONTROLLER_H
#define CLOSED_LOOP_CONTROLLER_H

#include <Arduino.h>
#include <tmcdriver.h>
#include <encoder.h>
#include <abstractcontroller.h>

class ClosedLoopController : public AbstractController
{
public:
    TMC5160Controller &stepper;
    uint32_t maxPollInterval = 50;
    bool enabled;
    double targetAngle;
    double tolerance = 0.1;
    double encoderOffset = 0.;
    double error;
    double limitA = 0.;
    double limitB = 360.;
    double calibrationDecay = 0.1;
    int calibrationSpeed = 5;
    bool hasLimits = false;
    bool hasCalibration = false;
    bool calibrationRunning = false;
    static const int calibrationSteps = 128;
    float calibrationOffsets[calibrationSteps];
    double calibrationStepperStartOffset = 0.;
    ClosedLoopController(TMC5160Controller &stepper, Encoder &encoder) : AbstractController(encoder), stepper(stepper) {}
    double mod(double a, double N) {return a - N*floor(a/N);}
    double angularDistance(double a, double b) {
        return mod(a - b + 180., 360.) - 180.;
    }
    void setAngle(double angle) {
        targetAngle = angle;
        double curAngle = getAngle();
        if (encoder.hasNewData()) {
            if (hasLimits) {
                double middle = mod((limitA + limitB) * 0.5, 360.);
                double interval = limitB - limitA;
                if (limitB < limitA) {
                    middle = mod(middle + 180., 360.);
                    interval = mod(interval, 360.);
                }
                double t = mod(targetAngle - middle + 180., 360.) - 180.;
                targetAngle = mod(max(min(t, interval*0.5), -interval*0.5) + middle, 360.);
                error = mod(targetAngle - middle + 180., 360.) - mod(curAngle - middle + 180., 360.);
                // ESP_LOGI("Controller", "Middle : %f`, Interval: %f, t: %f, Target: %f, To Go: %f\n", middle, interval, t, targetAngle, error);
            }
            else error = mod(targetAngle - curAngle + 180., 360.) - 180.;
            // ESP_LOGI("Controller", "Command : %f, Target: %f, Current: %f, To Go: %f\n", angle, targetAngle, curAngle, error);
            if (abs(error) > tolerance && enabled) {
                stepper.setMaxSpeed();
                stepper.moveR(error);
            }
        }
    }
    double getAngle(){
        if (hasCalibration) return getCalibratedAngle();
        else return mod(encoder.getAngle()+encoderOffset, 360.);
    }
    double lerp(double a, double b, double t) {
        return b * t + a * (1. - t);
    }
    void run() {
        if (calibrationRunning) runCalibration();
        else if (enabled && millis() - lastPoll >= maxPollInterval) {
            setAngle(targetAngle);
            lastPoll = millis();
        }
    }
    void startCalibration() {
        if (!calibrationRunning) {
            calibrationStepperStartOffset = angularDistance(stepper.getAngle(), encoder.getAngle());
            stepper.setSpeed(calibrationSpeed);
            if (hasLimits) setAngle(limitA);
            calibrationRunning = true;
        }
    }
    void stopCalibration() {
        if (calibrationRunning) {
            stepper.stop();
            calibrationRunning = false;
        }
    }
    void resetCalibration() {
        for (int i = 0; i < calibrationSteps; i++) {
            calibrationOffsets[i] = 0.;
        }
    }
    void setCalibrationSpeed(int speed) {
        calibrationSpeed = speed;
        if (calibrationRunning) stepper.setSpeed(calibrationSpeed);
    }
    void setEncoderOffset(double offset) {
        double offsetDiff = offset - encoderOffset;
        encoderOffset = offset;
        limitA = mod(limitA + offsetDiff, 360.);
        limitB = mod(limitB + offsetDiff, 360.);
    }
private:
    double getCalibratedAngle() {
        double rawAngle = encoder.getAngle();
        double index = rawAngle*calibrationSteps/360.;
        double current = calibrationOffsets[int(floor(index)) % calibrationSteps];
        double next = calibrationOffsets[int(ceil(index)) % calibrationSteps];
        double offset = lerp(current, next, mod(index, 1.));
        return mod(rawAngle + offset + encoderOffset, 360.);
    }
    void runCalibration() {
        double rawAngle = encoder.getAngle();
        if (encoder.hasNewData()) {
            double stepperAngle = stepper.getAngle();
            double offset = mod(stepperAngle - rawAngle - calibrationStepperStartOffset + 180., 360.) - 180.;
            ESP_LOGI("Calibration", "Offset %f, Encoder %f, Stepper %f", offset, rawAngle, stepperAngle);
            float index = rawAngle*calibrationSteps/360.;
            int current = int(floor(index)) % calibrationSteps;
            if (calibrationOffsets[current] == 0.) {
                calibrationOffsets[current] = offset;
                ESP_LOGI("Calibration", "current %d", current);
            }
            else {
                int next = int(ceil(index)) % calibrationSteps;
                double fract = mod(index, 1.);
                calibrationOffsets[current] = lerp(calibrationOffsets[current], offset, calibrationDecay - fract * calibrationDecay);
                if (calibrationOffsets[next] != 0.) calibrationOffsets[next] = lerp(calibrationOffsets[next], offset, fract * calibrationDecay);
            }
        }
        if (hasLimits) {
            if (abs(error) < tolerance) {
                if (abs(angularDistance(getAngle(), limitA)) > abs(angularDistance(getAngle(), limitB))) {
                    setAngle(limitA);
                    ESP_LOGI("Calibration", "Goto A");
                }
                else {
                    setAngle(limitB);
                    ESP_LOGI("Calibration", "Goto B");
                }
            }
            else setAngle(targetAngle);
        }
    }
    uint32_t lastPoll = 0;
};
#endif