#ifndef ADXL345_H
#define ADXL345_H

#include <Arduino.h>
#include <Wire.h>

class ADXL345 {
public:
    explicit ADXL345(TwoWire &wire = Wire, uint8_t address = 0x53);

    bool begin(int sda = -1, int scl = -1, uint32_t clockHz = 400000);
    bool isPresent();
    bool readAcceleration(float &ax, float &ay, float &az);

private:
    TwoWire &_wire;
    uint8_t _addr;

    bool writeReg(uint8_t reg, uint8_t value);
    bool readRegs(uint8_t reg, uint8_t *buf, size_t len);
};

#endif
