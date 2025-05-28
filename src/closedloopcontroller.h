#ifndef CLOSED_LOOP_CONTROLLER_H
#define CLOSED_LOOP_CONTROLLER_H

#include <Arduino.h>
#include <tmcdriver.h>
#include <encoder.h>
#include <abstractcontroller.h>
#include <ClosedLoopControllerService.h>

class ClosedLoopController : public AbstractController
{
public:
    TMC5160Controller &stepper;
    uint32_t maxPollInterval = 50;
    double calibrationDecay = 0.1;
    int calibrationSpeed = 5;
    bool hasCalibration = false;
    bool calibrationRunning = false;
    static const int calibrationSteps = 128;
    float calibrationOffsets[calibrationSteps];
    double calibrationStepperStartOffset = 0.;
    ClosedLoopController(TMC5160Controller &stepper, Encoder &encoder) : AbstractController(encoder), stepper(stepper) {}
    uint8_t getType() const override { return 1; }
    void init() {
        getAngle();
        if (encoder.hasNewData()) {
            targetAngle =  getAngle();
            run();
        }
    }
    void setAngle(double angle) {
        setTarget(angle);
        calcError();
        if (enabled && abs(error) > tolerance && encoder.hasNewData()) {
            stepper.setMaxSpeed();
            stepper.moveR(error);
        }
    }
    double getAngle(){
        if (hasCalibration) return getCalibratedAngle();
        else return mod(encoder.getAngle()+encoderOffset, 360.);
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