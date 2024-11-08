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
    bool hasCalibration = false;
    std::vector<int8_t> calibrationOffsets;
    double calibrationMaxAmplitude = 0.;
    double limitA = 0.;
    double limitB = 360.;
    ClosedLoopController(TMC5160Controller &stepper, Encoder &encoder) : stepper(stepper), encoder(encoder) {}
    double mod(double a, double N) {return a - N*floor(a/N);}
    void setAngle(double angle) {
        targetAngle = angle;
        double curAngle = encoder.getAngle();
        double toGo = 0.;
        if (hasLimits) {
            double middle = mod((limitA + limitB) * 0.5, 360.);
            double interval = limitB - limitA;
            if (limitB < limitA) {
                middle = mod(middle + 180., 360.);
                interval = mod(interval, 360.);
            }
            double t = mod(targetAngle - middle + 180., 360.) - 180.;
            targetAngle = mod(max(min(t, interval*0.5), -interval*0.5) + middle, 360.);
            toGo = mod(targetAngle - middle + 180., 360.) - mod(curAngle - middle + 180., 360.);
            // Serial.printf("Middle : %f`, Interval: %f, t: %f, Target: %f, To Go: %f\n", middle, interval, t, targetAngle, toGo);
        }
        else toGo = mod(targetAngle - curAngle + 180., 360.) - 180.;
        // Serial.printf("Command : %f, Target: %f, Current: %f, To Go: %f\n", angle, targetAngle, curAngle, toGo);
        if (abs(toGo) > tolerance && encoder.hasNewData()) stepper.moveR(toGo);
    }
    double getAngle(){
        if (hasCalibration) return getCalibratedAngle();
        else return encoder.getAngle();
    }
    double getCalibratedAngle() {
        double readAngle = encoder.getAngle();
        double index = readAngle*360./calibrationOffsets.size();
        double before = calibrationOffsets[floor(index)] * calibrationMaxAmplitude;
        double after = calibrationOffsets[ceil(index)] * calibrationMaxAmplitude;
        double offset = after * fmod(index, 1) + before * (1. - fmod(index, 1));
        return readAngle + offset;
    }
    void run() {
        if (enabled && millis() - lastPoll >= maxPollInterval) {
            setAngle(targetAngle);
            lastPoll = millis();
        }
    }
private:
    uint32_t lastPoll = 0;
};
#endif