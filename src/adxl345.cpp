#include "adxl345.h"

namespace {
constexpr uint8_t REG_DEVID = 0x00;
constexpr uint8_t REG_BW_RATE = 0x2C;
constexpr uint8_t REG_POWER_CTL = 0x2D;
constexpr uint8_t REG_DATA_FORMAT = 0x31;
constexpr uint8_t REG_DATAX0 = 0x32;
constexpr uint8_t DEVID_EXPECTED = 0xE5;

constexpr float G_PER_LSB = 0.0039f; // full resolution ~3.9 mg/LSB
constexpr float MS2_PER_G = 9.80665f;
}

ADXL345::ADXL345(TwoWire &wire, uint8_t address) : _wire(wire), _addr(address) {}

bool ADXL345::begin(int sda, int scl, uint32_t clockHz) {
    if (sda >= 0 && scl >= 0) {
        _wire.begin(sda, scl);
    } else {
        _wire.begin();
    }
    _wire.setClock(clockHz);

    if (!isPresent()) {
        return false;
    }

    if (!writeReg(REG_BW_RATE, 0x0A)) return false;      // 100 Hz
    if (!writeReg(REG_DATA_FORMAT, 0x0B)) return false;  // full res, +/-16g
    if (!writeReg(REG_POWER_CTL, 0x08)) return false;    // measure mode
    return true;
}

bool ADXL345::isPresent() {
    uint8_t id = 0;
    if (!readRegs(REG_DEVID, &id, 1)) return false;
    return id == DEVID_EXPECTED;
}

bool ADXL345::readAcceleration(float &ax, float &ay, float &az) {
    uint8_t buf[6] = {0};
    if (!readRegs(REG_DATAX0, buf, sizeof(buf))) return false;

    int16_t rawX = (int16_t)((buf[1] << 8) | buf[0]);
    int16_t rawY = (int16_t)((buf[3] << 8) | buf[2]);
    int16_t rawZ = (int16_t)((buf[5] << 8) | buf[4]);

    float scale = G_PER_LSB * MS2_PER_G;
    ax = rawX * scale;
    ay = rawY * scale;
    az = rawZ * scale;
    return true;
}

bool ADXL345::writeReg(uint8_t reg, uint8_t value) {
    _wire.beginTransmission(_addr);
    _wire.write(reg);
    _wire.write(value);
    return _wire.endTransmission() == 0;
}

bool ADXL345::readRegs(uint8_t reg, uint8_t *buf, size_t len) {
    _wire.beginTransmission(_addr);
    _wire.write(reg);
    if (_wire.endTransmission(false) != 0) return false;

    size_t read = _wire.requestFrom((int)_addr, (int)len);
    if (read != len) return false;

    for (size_t i = 0; i < len; i++) {
        buf[i] = _wire.read();
    }
    return true;
}
