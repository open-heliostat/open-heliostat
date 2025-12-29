#ifndef ACCELEROMETER_CALIBRATION_H
#define ACCELEROMETER_CALIBRATION_H

#include <Arduino.h>

// Parameter layout:
// p[0]  azimuth encoder offset (rad)
// p[1]  elevation encoder offset (rad)
// p[2]  accelerometer misalignment roll
// p[3]  accelerometer misalignment pitch
// p[4]  accelerometer bias X
// p[5]  accelerometer bias Y
// p[6]  accelerometer bias Z
// p[7]  accelerometer scale X
// p[8]  accelerometer scale Y
// p[9]  accelerometer scale Z

constexpr uint32_t ACCEL_CALIB_MAGIC = 0xC011BEEF;
constexpr int ACCEL_CALIB_PARAM_COUNT = 10;

struct Sample {
    float az;   // rad
    float el;   // rad
    float ax;   // m/s^2
    float ay;
    float azz;
};

struct CalibData {
    uint32_t magic;
    float p[ACCEL_CALIB_PARAM_COUNT];
    uint32_t crc;
};

bool checkObservability(const Sample* samples, int count);
void predictSample(const Sample& sample, const float* params, float out[3]);
void calibrate(const Sample* data, int count, float* params);

uint32_t computeCalibCRC(const float* params);
bool loadCalib(const CalibData& stored, float* params);
void storeCalib(const float* params, CalibData& out);

#endif
