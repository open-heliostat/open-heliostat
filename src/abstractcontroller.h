#ifndef ABSTRACTCONTROLLER_H
#define ABSTRACTCONTROLLER_H
#include <encoder.h>

class AbstractController {
public:
    AbstractController(Encoder &encoder) : encoder(encoder) {}
    virtual ~AbstractController() {}

    // Core control methods
    virtual void run() = 0;
    virtual double getAngle() = 0;
    virtual void setAngle(double angle) = 0;
    
    Encoder &encoder;
    double targetAngle = 0;
};

#endif
