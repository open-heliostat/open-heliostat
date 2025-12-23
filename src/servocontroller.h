#ifndef SERVOMOTORCLASS
#define SERVOMOTORCLASS
#include <Arduino.h>
#include "dcmotor.h"
#include "encoder.h"
#include <abstractcontroller.h>
#include "teleplot.h"

struct Servo_Driver : public AbstractController
{
    Motor_Driver &motor;
    bool plot = false;
    double derivative = 0;
    double P = 10;
    double I = 0;
    double D = 0;
    double S = 0;
    double integral = 0;
    double lastError = 0;
    double curGain = 0;
    double lastDerivative = 0;
	Servo_Driver(Motor_Driver &mot, Encoder &encoder)
        : AbstractController(encoder), motor{mot} {}
    uint8_t getType() const override { return 2; }
    void run() {
        if (enabled) {
            double curAngle = getAngle();
            calcError();
            double p = double(error)/10. * P;

            // if (abs(p) < 0.25) integral += double(error)/1000.;
            // else integral = 0.;
            // integral = min(max(integral+error, -P), P);
            // derivative = double(lastError - error) * 1. + lastDerivative * 0.;
            // lastDerivative = derivative;
            // double speed = lastDestination - destination;
            // lastDestination = destination;
            double result = p + integral * I + derivative * D;// + speed * S;
            curGain = min(max(result, -1.), 1.);
            
            if (plot) {
                TELEPLOT_SEND("angle", curAngle);
                TELEPLOT_SEND("target", targetAngle);
                TELEPLOT_SEND("error", error);
                TELEPLOT_SEND("p_term", p);
                TELEPLOT_SEND("output", curGain);
            }
            
            if (abs(error) < tolerance) {
                curGain = 0;
                integral = 0;
            }
            motor.setSpeed(curGain);
            lastError = error;
        }
    }
    void setAngle(double angle) override {
        setTarget(angle);
        calcError();
        if (enabled && abs(error) > tolerance && encoder.hasNewData()) {
            run();
        }
    }
    void init() override {
        getAngle();
        if (encoder.hasNewData()) {
            targetAngle = getAngle();
            run();
        }
    }
};
#endif