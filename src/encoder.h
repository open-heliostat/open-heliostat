#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include <Wire.h>
#include <driver/i2c.h>

class Encoder
{
public:
    double angle;
    bool invert = false;
    bool error = false;
    int _SDA, _SCL;
    Encoder(int SDA = SDA, int SCL = SCL, TwoWire &I2C_ = Wire) : I2C(I2C_), _SDA(SDA), _SCL(SCL) {
    }
    void init() {
        I2C.begin(_SDA, _SCL);
        I2C.setClock(400000);
        I2C.setTimeOut(10);
    }
    double getAngle() {
        update();
        return angle;
    }
    bool hasNewData() {
        return newData && millis() - lastPoll <= maxPollInterval;
    }
    bool update() {
        uint32_t now = millis();
        if (now - lastPoll >= maxPollInterval) {
            int value = readEncoder();
            lastPoll = now;
            if (value > 0 && value < 16384) {
                angle = value*360./16384.;
                if (invert) angle = 360. - angle;
                newData = true;
                error = false;
                failCount = 0;
                return true;
            }
            else {
                Serial.printf("Bad I2C Data : %d, Delay : %d\n", value, millis()-now);
                newData = false;
                error = true;
                failCount++;
                if (failCount >= recoverThreshold && millis() - lastRecoverMs > recoverCooldownMs) {
                    recoverBus();
                    lastRecoverMs = millis();
                    failCount = 0;
                }
            }
        }
        return newData;
    }
    int readEncoder() {
        uint32_t t0 = millis();
        const i2c_port_t port = (&I2C == &Wire1) ? I2C_NUM_1 : I2C_NUM_0;
        uint8_t buff[3] = {0, 0, 0};

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        // Write register pointer 0x02
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (0x06 << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, 0x02, true);
        // Repeated start + read 3 bytes
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (0x06 << 1) | I2C_MASTER_READ, true);
        i2c_master_read(cmd, buff, 3, I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);

        esp_err_t res = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(maxTxnMs));
        i2c_cmd_link_delete(cmd);

        if (res == ESP_OK) {
            int value = (256 * buff[1] + buff[2]) / 4;
            maybeRecoverAfterTxn(t0, false);
            return value;
        }

        // Timeout or bus error
        maybeRecoverAfterTxn(t0, res == ESP_ERR_TIMEOUT);
        return -1;
    }
private:
    TwoWire &I2C;
    uint32_t maxPollInterval = 20;
    uint32_t lastPoll = 0;
    bool newData = false;
    uint8_t failCount = 0;
    const uint8_t recoverThreshold = 3;
    uint32_t lastRecoverMs = 0;
    const uint32_t recoverCooldownMs = 100;
    const uint32_t maxTxnMs = 20; // guard against long Wire stalls
    bool recovering = false;

    void recoverBus() {
        if (recovering) return;
        recovering = true;
        // Try to free the bus if a device holds SDA/SCL low, then re-init I2C.
        pinMode(_SCL, OUTPUT);
        pinMode(_SDA, INPUT_PULLUP);
        for (int i = 0; i < 9; i++) {
            digitalWrite(_SCL, HIGH);
            delayMicroseconds(5);
            digitalWrite(_SCL, LOW);
            delayMicroseconds(5);
        }
        pinMode(_SDA, INPUT_PULLUP);
        digitalWrite(_SCL, HIGH);
        delayMicroseconds(5);
        I2C.end();
        I2C.begin(_SDA, _SCL);
        I2C.setClock(100000);
        I2C.setTimeOut(10);
        recovering = false;
    }

    void maybeRecoverAfterTxn(uint32_t t0, bool force = false) {
        uint32_t dt = millis() - t0;
        if ((force || dt > maxTxnMs) && millis() - lastRecoverMs > recoverCooldownMs) {
            recoverBus();
            lastRecoverMs = millis();
            failCount = 0;
        }
    }
};
#endif