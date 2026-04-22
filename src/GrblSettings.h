#pragma once

#include <map>
#include <sstream>
#include <string>

namespace grbl {

/// All GRBL 1.1 `$$` settings as typed fields.
///
/// Populate by calling parseBlock() with the raw text you get back from `$$`.
/// Write individual changes back by calling formatLine() and sending the result
/// to GrblSender::sendImmediateLine().
///
/// Only the fields most commonly needed for machine setup are broken out here.
/// Every other setting is also available via the raw `all` map (key = setting
/// number, value = raw string as received from GRBL).
struct GrblSettings {

    // -----------------------------------------------------------------------
    // Motion
    // -----------------------------------------------------------------------

    int   stepPulseUsec       = 10;     ///< $0  step pulse width (µs)
    int   stepIdleDelayMsec   = 25;     ///< $1  stepper disable delay after idle (ms)
    float junctionDeviation   = 0.02f;  ///< $11 cornering deviation (mm)
    float arcTolerance        = 0.002f; ///< $12 arc interpolation tolerance (mm)

    // -----------------------------------------------------------------------
    // Limits & homing
    // -----------------------------------------------------------------------

    bool  softLimitsEnabled   = false;  ///< $20
    bool  hardLimitsEnabled   = false;  ///< $21
    bool  homingEnabled       = false;  ///< $22
    float homingFeedMmMin     = 50.f;   ///< $24
    float homingSeekMmMin     = 635.f;  ///< $25
    int   homingDebounceMsec  = 250;    ///< $26
    float homingPullOffMm     = 1.f;    ///< $27

    // -----------------------------------------------------------------------
    // Steps per mm
    // -----------------------------------------------------------------------

    float stepsPerMmX  = 80.f;   ///< $100
    float stepsPerMmY  = 80.f;   ///< $101
    float stepsPerMmZ  = 80.f;   ///< $102

    // -----------------------------------------------------------------------
    // Max rates (mm/min)
    // -----------------------------------------------------------------------

    float maxRateX = 1000.f;  ///< $110
    float maxRateY = 1000.f;  ///< $111
    float maxRateZ = 1000.f;  ///< $112

    // -----------------------------------------------------------------------
    // Acceleration (mm/sec²)
    // -----------------------------------------------------------------------

    float accelX = 50.f;  ///< $120
    float accelY = 50.f;  ///< $121
    float accelZ = 50.f;  ///< $122

    // -----------------------------------------------------------------------
    // Max travel (mm)  — used by soft limits
    // -----------------------------------------------------------------------

    float maxTravelX = 200.f;  ///< $130
    float maxTravelY = 200.f;  ///< $131
    float maxTravelZ = 200.f;  ///< $132

    // -----------------------------------------------------------------------
    // Raw settings map — every $N=value pair from `$$`, key = N
    // -----------------------------------------------------------------------

    std::map<int, std::string> all;

    // -----------------------------------------------------------------------
    // Parsing
    // -----------------------------------------------------------------------

    /// Feed the raw text block returned by GRBL's `$$` command.
    /// Returns the number of settings parsed.
    int parseBlock(const std::string& text) {
        all.clear();
        std::istringstream ss(text);
        std::string line;
        int count = 0;
        while (std::getline(ss, line)) {
            if (line.empty() || line[0] != '$') continue;
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            // Key: digits between '$' and '='
            std::string keyStr = line.substr(1, eq - 1);
            if (keyStr.empty()) continue;
            bool allDigits = true;
            for (char ch : keyStr) { if (!std::isdigit((unsigned char)ch)) { allDigits = false; break; } }
            if (!allDigits) continue;
            int key = std::stoi(keyStr);
            // Value: everything up to the first space or '(' (GRBL appends a comment)
            std::string val = line.substr(eq + 1);
            size_t end = val.find_first_of(" \t(");
            if (end != std::string::npos) val.resize(end);
            all[key] = val;
            ++count;
        }
        applyFromMap();
        return count;
    }

    /// Produce a single `$N=value` command string ready to send back to GRBL.
    static std::string formatLine(int key, const std::string& value) {
        return "$" + std::to_string(key) + "=" + value;
    }
    static std::string formatLine(int key, int value) {
        return "$" + std::to_string(key) + "=" + std::to_string(value);
    }
    static std::string formatLine(int key, float value) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "$%d=%.3f", key, value);
        return std::string(buf);
    }

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /// Steps-per-mm formula for a belt/pulley drive:
    ///   (motorSteps * microSteps) / (beltPitchMm * pulleyTeeth)
    static float stepsPerMm(int motorSteps, int microSteps, float beltPitchMm, int pulleyTeeth) {
        return (float)(motorSteps * microSteps) / (beltPitchMm * (float)pulleyTeeth);
    }

private:
    void applyFromMap() {
        auto getFloat = [&](int k, float def) -> float {
            auto it = all.find(k);
            if (it == all.end()) return def;
            try { return std::stof(it->second); } catch (...) { return def; }
        };
        auto getInt = [&](int k, int def) -> int {
            auto it = all.find(k);
            if (it == all.end()) return def;
            try { return std::stoi(it->second); } catch (...) { return def; }
        };
        auto getBool = [&](int k, bool def) -> bool {
            return getInt(k, def ? 1 : 0) != 0;
        };

        stepPulseUsec      = getInt(0,  stepPulseUsec);
        stepIdleDelayMsec  = getInt(1,  stepIdleDelayMsec);
        junctionDeviation  = getFloat(11, junctionDeviation);
        arcTolerance       = getFloat(12, arcTolerance);
        softLimitsEnabled  = getBool(20, softLimitsEnabled);
        hardLimitsEnabled  = getBool(21, hardLimitsEnabled);
        homingEnabled      = getBool(22, homingEnabled);
        homingFeedMmMin    = getFloat(24, homingFeedMmMin);
        homingSeekMmMin    = getFloat(25, homingSeekMmMin);
        homingDebounceMsec = getInt(26,  homingDebounceMsec);
        homingPullOffMm    = getFloat(27, homingPullOffMm);
        stepsPerMmX        = getFloat(100, stepsPerMmX);
        stepsPerMmY        = getFloat(101, stepsPerMmY);
        stepsPerMmZ        = getFloat(102, stepsPerMmZ);
        maxRateX           = getFloat(110, maxRateX);
        maxRateY           = getFloat(111, maxRateY);
        maxRateZ           = getFloat(112, maxRateZ);
        accelX             = getFloat(120, accelX);
        accelY             = getFloat(121, accelY);
        accelZ             = getFloat(122, accelZ);
        maxTravelX         = getFloat(130, maxTravelX);
        maxTravelY         = getFloat(131, maxTravelY);
        maxTravelZ         = getFloat(132, maxTravelZ);
    }
};

} // namespace grbl
