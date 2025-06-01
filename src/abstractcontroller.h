#ifndef ABSTRACTCONTROLLER_H
#define ABSTRACTCONTROLLER_H
#include <encoder.h>

class AbstractController {
public:
    AbstractController(Encoder &encoder) : encoder(encoder) {}
    virtual ~AbstractController() {}

    // Core control methods
    virtual void run() = 0;
    virtual void init() = 0;
    virtual void setAngle(double angle) = 0;
    virtual uint8_t getType() const { return 0; }
    
    Encoder &encoder;
    bool enabled;
    bool hasLimits = false;
    double targetAngle;
    double tolerance = 0.1;
    double encoderOffset = 0.;
    double error;
    double limitA = 0.;
    double limitB = 360.;

    double mod(double a, double N) {return a - N*floor(a/N);}
    double angularDistance(double a, double b) {
        return mod(a - b + 180., 360.) - 180.;
    }
    double lerp(double a, double b, double t) {
        return b * t + a * (1. - t);
    }
    double calcMiddle() {
        double middle = (mod(limitA, 360.) + mod(limitB, 360.)) * 0.5;
        if (limitB < limitA) {
            middle = mod(middle + 180., 360.);
        }
        return middle;
    }
    double calcInterval() {
        double interval = mod(limitB - limitA, 360.);
        return interval;
    }
    double getTarget() {
        return targetAngle;
    }
    void setTarget(double angle) {
        if (hasLimits) {
            double middle = calcMiddle();
            double interval = calcInterval();
            // double t = mod(targetAngle - middle + 180., 360.) - 180.;
            double t = angularDistance(angle, middle);
            targetAngle = mod(max(min(t, interval*0.5), -interval*0.5) + middle, 360.);
        }
        else targetAngle = mod(angle, 360.);
    }
    virtual double getAngle(){
        return mod(encoder.getAngle()+encoderOffset, 360.);
    }
    void calcError() {
        double curAngle = getAngle();
        if (hasLimits) {
            double middle = calcMiddle();
            double interval = calcInterval();
            error = mod(targetAngle - middle + 180., 360.) - mod(curAngle - middle + 180., 360.);
        }
        else error = angularDistance(targetAngle, curAngle);
    }
    void setEncoderOffset(double offset) {
        double offsetDiff = offset - encoderOffset;
        encoderOffset = offset;
        limitA = mod(limitA + offsetDiff, 360.);
        limitB = mod(limitB + offsetDiff, 360.);
    }
};

#endif
