#ifndef DCMOTORCLASS
#define DCMOTORCLASS
#include <Arduino.h>

class Motor_Driver
{
public:
	const uint8_t EN;
	const uint8_t IN1;
	const uint8_t IN2;
    const uint8_t CHAN;
	bool isRunning = false;
	bool sDirection = false;
    bool invert = false;
    double speed = 0;
    double shape = 1.;
    double minVal = 0.15;
	int duty = 0;
    const int maxVal = 4096;
	Motor_Driver(uint8_t EN, uint8_t IN1, uint8_t IN2, uint8_t CHAN)
        : EN{EN}, IN1{IN1}, IN2{IN2}, CHAN{CHAN}
    {
    }
    void init() {
        // pinMode(EN, OUTPUT);
        ledcSetup(CHAN, 15000, 12);
	    ledcAttachPin(EN, CHAN);
        pinMode(IN1, OUTPUT);
        pinMode(IN2, OUTPUT);
        setDirection(sDirection);
    }
	void setDuty(int d) {
        d = min(max(d, 0), maxVal);
        duty = d;
        ledcWrite(CHAN, d);
    }
    void setSpeed(double s) {
        setDirection(s < 0);
        int dut = (pow(abs(s), shape) * (1. - minVal) + (abs(s) < 0.001 ? 0. : minVal)) * maxVal;
        setDuty(dut);
        speed = s;
    }
    double getSpeed() {
        return speed;
    }
    void setMin(double m) {
        minVal = m;
    }
    double getMin() {
        return minVal;
    }
	int getDuty() { return duty; }
	void setDirection(bool dir) {
        sDirection = dir;
        if (invert) {
            digitalWrite(IN1, !sDirection);
            digitalWrite(IN2, sDirection);
        }
        else {
            digitalWrite(IN1, sDirection);
            digitalWrite(IN2, !sDirection);
        }
    }
	bool getDirection() { return sDirection; }
};
#endif
