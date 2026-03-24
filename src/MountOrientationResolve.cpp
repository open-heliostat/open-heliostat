#include "MountOrientationResolve.h"

#include <geometry.h>

#include <algorithm>

namespace {
constexpr float kMinNorm = 0.1f;
constexpr int kMaxFitSamples = 16;
constexpr double kCandidateRmsTieTolerance = 0.02;
constexpr double kCandidateAnglesDeg[] = {0.0, 90.0, 180.0, -90.0};

struct CandidateResult {
    double tiltDeg = 0.0;
    double tiltAzimuthDeg = 0.0;
    double sensorRollRad = 0.0;
    double sensorPitchRad = 0.0;
    double rms = 1e9;
    bool valid = false;
};

struct TiltEstimate {
    double tiltDeg = 0.0;
    double tiltAzimuthDeg = 0.0;
};

double normalizeAzimuthDeg(double azimuthDeg)
{
    double normalized = fmod(azimuthDeg, 360.0);
    if (normalized < 0.0) {
        normalized += 360.0;
    }
    return normalized;
}

double circularCoverageDeg(const MountOrientationObservation *observations, int count)
{
    if (!observations || count <= 0) {
        return 0.0;
    }

    double azimuths[kMaxFitSamples] = {};
    int used = 0;
    for (int i = 0; i < count && used < kMaxFitSamples; i++) {
        azimuths[used++] = normalizeAzimuthDeg(observations[i].azDeg);
    }
    if (used < 2) {
        return 0.0;
    }

    std::sort(azimuths, azimuths + used);
    double maxGap = 0.0;
    for (int i = 0; i < used; i++) {
        double current = azimuths[i];
        double next = azimuths[(i + 1) % used];
        if (i == used - 1) {
            next += 360.0;
        }
        maxGap = std::max(maxGap, next - current);
    }
    return 360.0 - maxGap;
}

vec3 sensorMountRotate(const vec3 &input, double rollRad, double pitchRad)
{
    double cr = cos(rollRad);
    double sr = sin(rollRad);
    double cp = cos(pitchRad);
    double sp = sin(pitchRad);

    return vec3{
        cp * input.x + sp * sr * input.y + sp * cr * input.z,
        cr * input.y - sr * input.z,
        -sp * input.x + cp * sr * input.y + cp * cr * input.z,
    };
}

vec3 inverseSensorMountRotate(const vec3 &input, double rollRad, double pitchRad)
{
    double cr = cos(rollRad);
    double sr = sin(rollRad);
    double cp = cos(pitchRad);
    double sp = sin(pitchRad);

    return vec3{
        cp * input.x - sp * input.z,
        sp * sr * input.x + cr * input.y + cp * sr * input.z,
        sp * cr * input.x - sr * input.y + cp * cr * input.z,
    };
}

vec3 measuredUnitVector(const MountOrientationObservation &observation)
{
    vec3 measured = {
        static_cast<double>(observation.ax),
        static_cast<double>(observation.ay),
        static_cast<double>(observation.azz)
    };
    double norm = measured.length();
    if (norm < kMinNorm) {
        return {0.0, 0.0, 0.0};
    }
    return measured / norm;
}

vec3 mountFrameAverageForSensorMount(const MountOrientationObservation *observations,
                                     int count,
                                     double sensorRollRad,
                                     double sensorPitchRad)
{
    vec3 accumulated = {0.0, 0.0, 0.0};
    int used = 0;

    for (int i = 0; i < count && i < kMaxFitSamples; i++) {
        vec3 mountVector = measuredUnitVector(observations[i]);
        if (mountVector.length() < kMinNorm) {
            continue;
        }
        mountVector = inverseSensorMountRotate(mountVector, sensorRollRad, sensorPitchRad);
        mountVector.rotZ(-degToRad(static_cast<double>(observations[i].azDeg)));
        mountVector.rotY(-degToRad(static_cast<double>(observations[i].elDeg)));
        accumulated = accumulated + mountVector.normalize();
        used++;
    }

    if (used == 0 || accumulated.length() < kMinNorm) {
        return {0.0, 0.0, 1.0};
    }
    return accumulated.normalize();
}

TiltEstimate tiltFromVector(const vec3 &vector)
{
    vec3 normalized = vector;
    normalized = normalized.normalize();
    double horizontal = sqrt(normalized.x * normalized.x + normalized.y * normalized.y);
    TiltEstimate params;
    params.tiltDeg = radToDeg(atan2(horizontal, normalized.z));
    params.tiltAzimuthDeg = horizontal < 1e-6 ? 0.0 : normalizeAzimuthDeg(radToDeg(atan2(normalized.y, normalized.x)));
    return params;
}

vec3 predictMeasurement(const MountOrientationObservation &observation,
                       double tiltDeg,
                       double tiltAzimuthDeg,
                       double sensorRollRad,
                       double sensorPitchRad)
{
    vec3 gravityMount = applyMountOrientationTransform({0.0, 0.0, 1.0}, tiltDeg, tiltAzimuthDeg);
    gravityMount.rotY(degToRad(static_cast<double>(observation.elDeg)));
    gravityMount.rotZ(degToRad(static_cast<double>(observation.azDeg)));
    return sensorMountRotate(gravityMount, sensorRollRad, sensorPitchRad).normalize();
}

double computeResidualRms(const MountOrientationObservation *observations,
                          int count,
                          double tiltDeg,
                          double tiltAzimuthDeg,
                          double sensorRollRad,
                          double sensorPitchRad)
{
    double error = 0.0;
    int used = 0;
    for (int i = 0; i < count && i < kMaxFitSamples; i++) {
        vec3 measured = measuredUnitVector(observations[i]);
        if (measured.length() < kMinNorm) {
            continue;
        }
        vec3 predicted = predictMeasurement(observations[i], tiltDeg, tiltAzimuthDeg, sensorRollRad, sensorPitchRad);
        vec3 residual = predicted - measured;
        error += residual.sqLength();
        used++;
    }
    if (used == 0) {
        return 1e9;
    }
    return sqrt(error / static_cast<double>(used));
}

void normaliseTilt(TiltEstimate &params)
{
    params.tiltAzimuthDeg = normalizeAzimuthDeg(params.tiltAzimuthDeg);
    if (params.tiltDeg < 0.0) {
        params.tiltDeg = -params.tiltDeg;
        params.tiltAzimuthDeg = normalizeAzimuthDeg(params.tiltAzimuthDeg + 180.0);
    }
}

bool isBetterCandidate(const CandidateResult &candidate, const CandidateResult &best)
{
    if (!best.valid) {
        return true;
    }
    if (candidate.rms + 1e-9 < best.rms) {
        return true;
    }
    if (fabs(candidate.rms - best.rms) <= kCandidateRmsTieTolerance && candidate.tiltDeg < best.tiltDeg) {
        return true;
    }
    return false;
}

CandidateResult solveFromRigidSensorCandidates(const MountOrientationObservation *observations, int count)
{
    CandidateResult best;

    for (double sensorRollDeg : kCandidateAnglesDeg) {
        for (double sensorPitchDeg : kCandidateAnglesDeg) {
            double sensorRollRad = degToRad(sensorRollDeg);
            double sensorPitchRad = degToRad(sensorPitchDeg);
            vec3 averageVector = mountFrameAverageForSensorMount(observations, count, sensorRollRad, sensorPitchRad);
            if (averageVector.length() < kMinNorm) {
                continue;
            }

            TiltEstimate tilt = tiltFromVector(averageVector);
            normaliseTilt(tilt);

            CandidateResult candidate;
            candidate.tiltDeg = tilt.tiltDeg;
            candidate.tiltAzimuthDeg = tilt.tiltAzimuthDeg;
            candidate.sensorRollRad = sensorRollRad;
            candidate.sensorPitchRad = sensorPitchRad;
            candidate.rms = computeResidualRms(observations,
                                               count,
                                               candidate.tiltDeg,
                                               candidate.tiltAzimuthDeg,
                                               candidate.sensorRollRad,
                                               candidate.sensorPitchRad);
            candidate.valid = true;

            if (isBetterCandidate(candidate, best)) {
                best = candidate;
            }
        }
    }

    return best;
}
}

bool checkMountOrientationObservability(const MountOrientationObservation *observations, int count)
{
    if (!observations || count < 8) {
        return false;
    }

    float minEl = 1e6f;
    float maxEl = -1e6f;

    for (int i = 0; i < count; i++) {
        minEl = min(minEl, observations[i].elDeg);
        maxEl = max(maxEl, observations[i].elDeg);
    }

    return circularCoverageDeg(observations, count) >= 180.0 && (maxEl - minEl) >= 20.0f;
}

bool estimateMountOrientation(const MountOrientationObservation *observations, int count, MountOrientationEstimate *outEstimate)
{
    if (!observations || !outEstimate || !checkMountOrientationObservability(observations, count)) {
        return false;
    }

    CandidateResult best = solveFromRigidSensorCandidates(observations, count);
    if (!best.valid) {
        return false;
    }

    vec3 gravityMount = applyMountOrientationTransform({0.0, 0.0, 1.0}, best.tiltDeg, best.tiltAzimuthDeg).normalize();

    if (gravityMount.length() < kMinNorm) {
        return false;
    }

    outEstimate->gravityX = static_cast<float>(gravityMount.x);
    outEstimate->gravityY = static_cast<float>(gravityMount.y);
    outEstimate->gravityZ = static_cast<float>(gravityMount.z);
    outEstimate->tiltDeg = static_cast<float>(best.tiltDeg);
    outEstimate->tiltAzimuthDeg = static_cast<float>(best.tiltAzimuthDeg);
    outEstimate->sensorRollDeg = static_cast<float>(radToDeg(best.sensorRollRad));
    outEstimate->sensorPitchDeg = static_cast<float>(radToDeg(best.sensorPitchRad));
    outEstimate->rms = static_cast<float>(best.rms);
    outEstimate->usedCount = static_cast<uint32_t>(std::min(count, kMaxFitSamples));
    return true;
}