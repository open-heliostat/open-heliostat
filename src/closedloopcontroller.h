#ifndef CLOSED_LOOP_CONTROLLER_H
#define CLOSED_LOOP_CONTROLLER_H

#include <Arduino.h>
#include <tmcdriver.h>
#include <encoder.h>

class ClosedLoopController
{
public:
    TMC5160Controller &stepper;
    Encoder &encoder;
    uint32_t maxPollInterval = 50;
    bool enabled;
    double targetAngle;
    double tolerance = 0.1;
    bool hasLimits = false;
    double limitA = 0.;
    double limitB = 360.;
    bool hasCalibration = false;
    bool calibrationRunning = false;
    static const int calibrationSteps = 1024;
    float calibrationOffsets[calibrationSteps];
    double calibrationStepperStartOffset = 0.;
    ClosedLoopController(TMC5160Controller &stepper, Encoder &encoder) : stepper(stepper), encoder(encoder) {}
    void setAngle(double angle) {
        targetAngle = angle;
        double curAngle = encoder.getAngle();
        double toGo = 0.;
        if (hasLimits) {
            double middle = fmod((limitA + limitB) * 0.5, 360.);
            double interval = limitB - limitA;
            if (limitB < limitA) {
                middle = fmod(middle + 180., 360.);
                interval = fmod(interval, 360.);
            }
            double t = fmod(targetAngle - middle + 180., 360.) - 180.;
            targetAngle = fmod(max(min(t, interval*0.5), -interval*0.5) + middle, 360.);
            toGo = fmod(targetAngle - middle + 180., 360.) - fmod(curAngle - middle + 180., 360.);
            // Serial.printf("Middle : %f`, Interval: %f, t: %f, Target: %f, To Go: %f\n", middle, interval, t, targetAngle, toGo);
        }
        else toGo = fmod(targetAngle - curAngle + 180., 360.) - 180.;
        // Serial.printf("Command : %f, Target: %f, Current: %f, To Go: %f\n", angle, targetAngle, curAngle, toGo);
        if (abs(toGo) > tolerance && encoder.hasNewData()) stepper.moveR(toGo);
    }
    double getAngle(){
        if (hasCalibration) return getCalibratedAngle();
        else return encoder.getAngle();
    }
    double getCalibratedAngle() {
        double readAngle = encoder.getAngle();
        double index = readAngle*360./calibrationSteps;
        double before = calibrationOffsets[int(floor(index))];
        double after = calibrationOffsets[int(ceil(index))];
        double offset = after * fmod(index, 1) + before * (1. - fmod(index, 1));
        return readAngle + offset;
    }
    void run() {
        if (calibrationRunning) runCalibration();
        else if (enabled && millis() - lastPoll >= maxPollInterval) {
            setAngle(targetAngle);
            lastPoll = millis();
        }
    }
    void startCalibration() {
        calibrationStepperStartOffset = stepper.getAngle() - encoder.getAngle();
        stepper.setSpeed(50);
        calibrationRunning = true;
    }
    void runCalibration() {
        double readAngle = encoder.getAngle();
        double stepperAngle = stepper.getAngle();
    }
private:
    uint32_t lastPoll = 0;
};
#endif