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
    virtual double getAngle() = 0;
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
    double getTarget() {
        return targetAngle;
    }
    void setTarget(double angle) {
        if (hasLimits) {
            double middle = mod((limitA + limitB) * 0.5, 360.);
            double interval = limitB - limitA;
            if (limitB < limitA) {
                middle = mod(middle + 180., 360.);
                interval = mod(interval, 360.);
            }
            double t = mod(targetAngle - middle + 180., 360.) - 180.;
            targetAngle = mod(max(min(t, interval*0.5), -interval*0.5) + middle, 360.);
        }
        else targetAngle = angle;
    }
    void calcError() {
        double curAngle = getAngle();
        if (hasLimits) {
            double middle = mod((limitA + limitB) * 0.5, 360.);
            double interval = limitB - limitA;
            if (limitB < limitA) {
                middle = mod(middle + 180., 360.);
                interval = mod(interval, 360.);
            }
            error = mod(targetAngle - middle + 180., 360.) - mod(curAngle - middle + 180., 360.);
        }
        else error = mod(targetAngle - curAngle + 180., 360.) - 180.;
    }
    void setEncoderOffset(double offset) {
        double offsetDiff = offset - encoderOffset;
        encoderOffset = offset;
        limitA = mod(limitA + offsetDiff, 360.);
        limitB = mod(limitB + offsetDiff, 360.);
    }
};

#endif
