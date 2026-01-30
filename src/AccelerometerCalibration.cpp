#include "AccelerometerCalibration.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static inline void mat3_mul(const float A[3][3], const float B[3][3], float C[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            C[i][j] = 0.0f;
            for (int k = 0; k < 3; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

static inline void mat3_vec(const float A[3][3], const float v[3], float r[3]) {
    for (int i = 0; i < 3; i++) {
        r[i] = A[i][0] * v[0] + A[i][1] * v[1] + A[i][2] * v[2];
    }
}

static inline void Rz(float a, float R[3][3]) {
    float c = cosf(a);
    float s = sinf(a);
    R[0][0] =  c; R[0][1] = -s; R[0][2] = 0.0f;
    R[1][0] =  s; R[1][1] =  c; R[1][2] = 0.0f;
    R[2][0] = 0.0f; R[2][1] = 0.0f; R[2][2] = 1.0f;
}

static inline void Ry(float a, float R[3][3]) {
    float c = cosf(a);
    float s = sinf(a);
    R[0][0] =  c; R[0][1] = 0.0f; R[0][2] =  s;
    R[1][0] = 0.0f; R[1][1] = 1.0f; R[1][2] = 0.0f;
    R[2][0] = -s; R[2][1] = 0.0f; R[2][2] =  c;
}

static inline void Rrp(float r, float p, float R[3][3]) {
    float cr = cosf(r);
    float sr = sinf(r);
    float cp = cosf(p);
    float sp = sinf(p);

    R[0][0] = cp;        R[0][1] = sp * sr;   R[0][2] = sp * cr;
    R[1][0] = 0.0f;      R[1][1] = cr;        R[1][2] = -sr;
    R[2][0] = -sp;       R[2][1] = cp * sr;   R[2][2] = cp * cr;
}

void predictSample(const Sample& sample, const float* params, float out[3]) {
    const float g[3] = {0.0f, 0.0f, 9.81f};

    float Rz_[3][3];
    float Ry_[3][3];
    float Rm_[3][3];
    float Rt[3][3];
    float R[3][3];

    Rz(sample.az + params[0], Rz_);
    Ry(sample.el + params[1], Ry_);
    Rrp(params[2], params[3], Rm_);

    mat3_mul(Rz_, Ry_, Rt);
    mat3_mul(Rm_, Rt, R);
    mat3_vec(R, g, out);

    out[0] = out[0] * params[7] + params[4];
    out[1] = out[1] * params[8] + params[5];
    out[2] = out[2] * params[9] + params[6];
}

void predictSampleOrientation(const Sample& sample, const float* params, float out[3]) {
    const float g[3] = {0.0f, 0.0f, 9.81f};

    float Rz_[3][3];
    float Ry_[3][3];
    float Rm_[3][3];
    float Rt[3][3];
    float R[3][3];

    Rz(sample.az + params[0], Rz_);
    Ry(sample.el + params[1], Ry_);
    Rrp(params[2], params[3], Rm_);

    mat3_mul(Rz_, Ry_, Rt);
    mat3_mul(Rm_, Rt, R);
    mat3_vec(R, g, out);
}

bool checkObservability(const Sample* samples, int count) {
    float minAz = 1e6f;
    float maxAz = -1e6f;
    float minEl = 1e6f;
    float maxEl = -1e6f;

    for (int i = 0; i < count; i++) {
        if (samples[i].az < minAz) minAz = samples[i].az;
        if (samples[i].az > maxAz) maxAz = samples[i].az;
        if (samples[i].el < minEl) minEl = samples[i].el;
        if (samples[i].el > maxEl) maxEl = samples[i].el;
    }

    if ((maxAz - minAz) < 1.0f) return false;  // < ~60°
    if ((maxEl - minEl) < 0.7f) return false;  // < ~40°
    return true;
}

static bool solve10(float A[ACCEL_CALIB_PARAM_COUNT][ACCEL_CALIB_PARAM_COUNT], float b[ACCEL_CALIB_PARAM_COUNT], float x[ACCEL_CALIB_PARAM_COUNT]) {
    float M[ACCEL_CALIB_PARAM_COUNT][ACCEL_CALIB_PARAM_COUNT + 1];
    for (int i = 0; i < ACCEL_CALIB_PARAM_COUNT; i++) {
        for (int j = 0; j < ACCEL_CALIB_PARAM_COUNT; j++) M[i][j] = A[i][j];
        M[i][ACCEL_CALIB_PARAM_COUNT] = b[i];
    }

    for (int i = 0; i < ACCEL_CALIB_PARAM_COUNT; i++) {
        int piv = i;
        for (int j = i + 1; j < ACCEL_CALIB_PARAM_COUNT; j++) {
            if (fabsf(M[j][i]) > fabsf(M[piv][i])) piv = j;
        }
        if (fabsf(M[piv][i]) < 1e-8f) return false;

        if (piv != i) {
            for (int k = i; k <= ACCEL_CALIB_PARAM_COUNT; k++) {
                float t = M[i][k];
                M[i][k] = M[piv][k];
                M[piv][k] = t;
            }
        }

        float d = M[i][i];
        for (int k = i; k <= ACCEL_CALIB_PARAM_COUNT; k++) M[i][k] /= d;

        for (int r = 0; r < ACCEL_CALIB_PARAM_COUNT; r++) {
            if (r == i) continue;
            float f = M[r][i];
            for (int k = i; k <= ACCEL_CALIB_PARAM_COUNT; k++) {
                M[r][k] -= f * M[i][k];
            }
        }
    }

    for (int i = 0; i < ACCEL_CALIB_PARAM_COUNT; i++) {
        x[i] = M[i][ACCEL_CALIB_PARAM_COUNT];
    }
    return true;
}

static bool solve4(float A[ACCEL_ORIENT_PARAM_COUNT][ACCEL_ORIENT_PARAM_COUNT], float b[ACCEL_ORIENT_PARAM_COUNT], float x[ACCEL_ORIENT_PARAM_COUNT]) {
    float M[ACCEL_ORIENT_PARAM_COUNT][ACCEL_ORIENT_PARAM_COUNT + 1];
    for (int i = 0; i < ACCEL_ORIENT_PARAM_COUNT; i++) {
        for (int j = 0; j < ACCEL_ORIENT_PARAM_COUNT; j++) M[i][j] = A[i][j];
        M[i][ACCEL_ORIENT_PARAM_COUNT] = b[i];
    }

    for (int i = 0; i < ACCEL_ORIENT_PARAM_COUNT; i++) {
        int piv = i;
        for (int j = i + 1; j < ACCEL_ORIENT_PARAM_COUNT; j++) {
            if (fabsf(M[j][i]) > fabsf(M[piv][i])) piv = j;
        }
        if (fabsf(M[piv][i]) < 1e-8f) return false;

        if (piv != i) {
            for (int k = i; k <= ACCEL_ORIENT_PARAM_COUNT; k++) {
                float t = M[i][k];
                M[i][k] = M[piv][k];
                M[piv][k] = t;
            }
        }

        float d = M[i][i];
        for (int k = i; k <= ACCEL_ORIENT_PARAM_COUNT; k++) M[i][k] /= d;

        for (int r = 0; r < ACCEL_ORIENT_PARAM_COUNT; r++) {
            if (r == i) continue;
            float f = M[r][i];
            for (int k = i; k <= ACCEL_ORIENT_PARAM_COUNT; k++) {
                M[r][k] -= f * M[i][k];
            }
        }
    }

    for (int i = 0; i < ACCEL_ORIENT_PARAM_COUNT; i++) {
        x[i] = M[i][ACCEL_ORIENT_PARAM_COUNT];
    }
    return true;
}

void calibrate(const Sample* data, int count, float* params) {
    const int P = ACCEL_CALIB_PARAM_COUNT;
    const float eps = 1e-4f;
    float lambda = 1e-3f;

    if (!checkObservability(data, count)) {
        printf("ERROR: Poor motion coverage\n");
        return;
    }

    for (int iter = 0; iter < 35; iter++) {
        float JTJ[ACCEL_CALIB_PARAM_COUNT][ACCEL_CALIB_PARAM_COUNT] = {0};
        float JTr[ACCEL_CALIB_PARAM_COUNT] = {0};
        float rms = 0.0f;

        for (int n = 0; n < count; n++) {
            float pred[3];
            float residual[3];
            predictSample(data[n], params, pred);

            residual[0] = pred[0] - data[n].ax;
            residual[1] = pred[1] - data[n].ay;
            residual[2] = pred[2] - data[n].azz;
            rms += residual[0] * residual[0] + residual[1] * residual[1] + residual[2] * residual[2];

            float J[P][3];
            for (int k = 0; k < P; k++) {
                float saved = params[k];
                params[k] += eps;
                float dp[3];
                predictSample(data[n], params, dp);
                params[k] = saved;

                J[k][0] = (dp[0] - pred[0]) / eps;
                J[k][1] = (dp[1] - pred[1]) / eps;
                J[k][2] = (dp[2] - pred[2]) / eps;
            }

            for (int i = 0; i < P; i++) {
                for (int j = 0; j < P; j++) {
                    JTJ[i][j] += J[i][0] * J[j][0] + J[i][1] * J[j][1] + J[i][2] * J[j][2];
                }
                JTr[i] += J[i][0] * residual[0] + J[i][1] * residual[1] + J[i][2] * residual[2];
            }
        }

        for (int i = 0; i < P; i++) {
            JTJ[i][i] *= (1.0f + lambda);
        }

        float dx[ACCEL_CALIB_PARAM_COUNT];
        if (!solve10(JTJ, JTr, dx)) {
            lambda *= 10.0f;
            continue;
        }

        for (int i = 0; i < P; i++) {
            params[i] -= dx[i];
        }

        printf("Iter %02d RMS %.3f lambda %.1e\n", iter, sqrtf(rms / count), lambda);
        lambda *= 0.7f;
    }
}

bool calibrateOrientation(const Sample* data, int count, float* params, float* outRms) {
    const int P = ACCEL_ORIENT_PARAM_COUNT;
    const float eps = 1e-4f;
    float lambda = 1e-3f;

    if (!checkObservability(data, count)) {
        printf("ERROR: Poor motion coverage\n");
        return false;
    }

    float bestRms = 1e9f;

    for (int iter = 0; iter < 35; iter++) {
        float JTJ[ACCEL_ORIENT_PARAM_COUNT][ACCEL_ORIENT_PARAM_COUNT] = {0};
        float JTr[ACCEL_ORIENT_PARAM_COUNT] = {0};
        float rms = 0.0f;

        for (int n = 0; n < count; n++) {
            float pred[3];
            float residual[3];
            predictSampleOrientation(data[n], params, pred);

            residual[0] = pred[0] - data[n].ax;
            residual[1] = pred[1] - data[n].ay;
            residual[2] = pred[2] - data[n].azz;
            rms += residual[0] * residual[0] + residual[1] * residual[1] + residual[2] * residual[2];

            float J[P][3];
            for (int k = 0; k < P; k++) {
                float saved = params[k];
                params[k] += eps;
                float dp[3];
                predictSampleOrientation(data[n], params, dp);
                params[k] = saved;

                J[k][0] = (dp[0] - pred[0]) / eps;
                J[k][1] = (dp[1] - pred[1]) / eps;
                J[k][2] = (dp[2] - pred[2]) / eps;
            }

            for (int i = 0; i < P; i++) {
                for (int j = 0; j < P; j++) {
                    JTJ[i][j] += J[i][0] * J[j][0] + J[i][1] * J[j][1] + J[i][2] * J[j][2];
                }
                JTr[i] += J[i][0] * residual[0] + J[i][1] * residual[1] + J[i][2] * residual[2];
            }
        }

        for (int i = 0; i < P; i++) {
            JTJ[i][i] *= (1.0f + lambda);
        }

        float dx[ACCEL_ORIENT_PARAM_COUNT];
        if (!solve4(JTJ, JTr, dx)) {
            lambda *= 10.0f;
            continue;
        }

        for (int i = 0; i < P; i++) {
            params[i] -= dx[i];
        }

        float rmsNow = sqrtf(rms / count);
        if (rmsNow < bestRms) bestRms = rmsNow;
        printf("Orient Iter %02d RMS %.3f lambda %.1e\n", iter, rmsNow, lambda);
        lambda *= 0.7f;
    }

    if (outRms) *outRms = bestRms;
    return true;
}

static uint32_t crc32_byte(uint32_t crc, uint8_t data) {
    crc ^= data;
    for (int i = 0; i < 8; i++) {
        uint32_t mask = -(crc & 1u);
        crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
    return crc;
}

uint32_t computeCalibCRC(const float* params) {
    uint32_t crc = 0xFFFFFFFFu;
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(params);
    for (size_t i = 0; i < sizeof(float) * ACCEL_CALIB_PARAM_COUNT; i++) {
        crc = crc32_byte(crc, bytes[i]);
    }
    return crc ^ 0xFFFFFFFFu;
}

bool loadCalib(const CalibData& stored, float* params) {
    if (stored.magic != ACCEL_CALIB_MAGIC) return false;
    uint32_t crc = computeCalibCRC(stored.p);
    if (crc != stored.crc) return false;
    memcpy(params, stored.p, sizeof(float) * ACCEL_CALIB_PARAM_COUNT);
    return true;
}

void storeCalib(const float* params, CalibData& out) {
    out.magic = ACCEL_CALIB_MAGIC;
    memcpy(out.p, params, sizeof(float) * ACCEL_CALIB_PARAM_COUNT);
    out.crc = computeCalibCRC(out.p);
}
