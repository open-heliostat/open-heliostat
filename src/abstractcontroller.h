#ifndef ABSTRACTCONTROLLER_H
#define ABSTRACTCONTROLLER_H
#include <encoder.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class AbstractController {
public:
    AbstractController(Encoder &encoder) : encoder(encoder) {
        _mutex = xSemaphoreCreateMutex();
    }
    virtual ~AbstractController() {
        vSemaphoreDelete(_mutex);
    }

    // Core control methods
    virtual void run() = 0;
    virtual void init() = 0;
    virtual void setAngle(double angle) = 0;
    virtual uint8_t getType() const { return 0; }
    
    Encoder &encoder;
    bool enabled;
    bool hasLimits = false;
    double tolerance = 0.1;
    double encoderOffset = 0.;
    double error;
    double limitA = 0.;
    double limitB = 360.;

protected:
    double targetAngle;
    SemaphoreHandle_t _mutex;

public:
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
        double t;
        xSemaphoreTake(_mutex, portMAX_DELAY);
        t = targetAngle;
        xSemaphoreGive(_mutex);
        return t;
    }
    void setTarget(double angle) {
        xSemaphoreTake(_mutex, portMAX_DELAY);
        if (hasLimits) {
            double middle = calcMiddle();
            double interval = calcInterval();
            // double t = mod(targetAngle - middle + 180., 360.) - 180.;
            double t = angularDistance(angle, middle);
            targetAngle = mod(max(min(t, interval*0.5), -interval*0.5) + middle, 360.);
        }
        else targetAngle = mod(angle, 360.);
        xSemaphoreGive(_mutex);
    }
    virtual double getAngle(){
        return mod(encoder.getAngle()+encoderOffset, 360.);
    }
    void calcError() {
        double curAngle = getAngle();
        double t = getTarget();
        if (hasLimits) {
            double middle = calcMiddle();
            double interval = calcInterval();
            error = mod(t - middle + 180., 360.) - mod(curAngle - middle + 180., 360.);
        }
        else error = angularDistance(t, curAngle);
    }
    void setEncoderOffset(double offset) {
        double offsetDiff = offset - encoderOffset;
        encoderOffset = offset;
        limitA = mod(limitA + offsetDiff, 360.);
        limitB = mod(limitB + offsetDiff, 360.);
    }
};

#endif
