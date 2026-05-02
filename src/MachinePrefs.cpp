#include "MachinePrefs.h"

#include "ofFileUtils.h"
#include "ofJson.h"
#include "ofLog.h"
#include "ofUtils.h"
#include <algorithm>

namespace grbl {

// ---------------------------------------------------------------------------
// MachineCapabilities factories
// ---------------------------------------------------------------------------

MachineCapabilities MachineCapabilities::grbl() {
    return MachineCapabilities{
        /*supportsRealtimeStatusQuery*/ true,
        /*supportsDollarSettings*/      true,
        /*supportsGrblJog*/             true,
        /*supportsJogCancel*/           true,
        /*supportsM114Position*/        false,
        /*supportsM115FirmwareInfo*/    false,
    };
}

MachineCapabilities MachineCapabilities::marlin() {
    return MachineCapabilities{
        /*supportsRealtimeStatusQuery*/ false,
        /*supportsDollarSettings*/      false,
        /*supportsGrblJog*/             false,
        /*supportsJogCancel*/           false,
        /*supportsM114Position*/        true,
        /*supportsM115FirmwareInfo*/    true,
    };
}

MachineCapabilities MachineCapabilities::custom() {
    return MachineCapabilities{
        /*supportsRealtimeStatusQuery*/ false,
        /*supportsDollarSettings*/      false,
        /*supportsGrblJog*/             false,
        /*supportsJogCancel*/           false,
        /*supportsM114Position*/        false,
        /*supportsM115FirmwareInfo*/    false,
    };
}

// ---------------------------------------------------------------------------
// Helpers (file-local)
// ---------------------------------------------------------------------------

namespace {

std::string resolveJsonPath(const std::string& jsonPath, bool jsonPathIsAbsolute) {
    if (jsonPath.empty()) {
        return ofToDataPath(MachinePrefs::defaultRelativePath(), true);
    }
    return jsonPathIsAbsolute ? jsonPath : ofToDataPath(jsonPath, true);
}

float jsonToFloat(const ofJson& j) {
    if (j.is_number_float())   return j.get<float>();
    if (j.is_number_integer()) return static_cast<float>(j.get<int>());
    return 0.f;
}

void readBool(const ofJson& j, const char* key, bool& out) {
    if (j.contains(key) && j[key].is_boolean()) {
        out = j[key].get<bool>();
    }
}

MachineCapabilities capsFromProfileString(const std::string& v) {
    const std::string s = ofToLower(v);
    if (s == "marlin" || s == "anycubic" || s == "3dp" || s == "fdm") {
        return MachineCapabilities::marlin();
    }
    if (s == "custom" || s == "other") {
        return MachineCapabilities::custom();
    }
    return MachineCapabilities::grbl();
}

std::string capsToProfileString(const MachineCapabilities& c) {
    if (c.supportsRealtimeStatusQuery && c.supportsGrblJog) return "grbl";
    if (c.supportsM114Position)                             return "marlin";
    return "custom";
}

} // namespace

// ---------------------------------------------------------------------------
// Envelope
// ---------------------------------------------------------------------------

void Envelope::clampXYZ(float& x, float& y, float& z) const {
    x = std::min(maxX, std::max(minX, x));
    y = std::min(maxY, std::max(minY, y));
    z = std::min(maxZ, std::max(minZ, z));
}

// ---------------------------------------------------------------------------
// MachinePrefs load / save
// ---------------------------------------------------------------------------

void MachinePrefs::load(const std::string& jsonPath, bool jsonPathIsAbsolute) {
    const std::string resolved = resolveJsonPath(jsonPath, jsonPathIsAbsolute);
    if (!ofFile::doesFileExist(resolved, false)) {
        if (jsonPath.empty()) {
            ofLogNotice("grbl::MachinePrefs") << "no file at " << resolved << ", using defaults";
        } else {
            ofLogWarning("grbl::MachinePrefs") << "no file at " << resolved << ", using defaults";
        }
        return;
    }

    const ofJson j = ofLoadJson(resolved);
    if (!j.is_object()) {
        ofLogWarning("grbl::MachinePrefs") << "invalid JSON at " << resolved << ", using defaults";
        return;
    }

    if (j.contains("baudRate") && j["baudRate"].is_number_integer()) {
        baudRate = j["baudRate"].get<int>();
    } else if (j.contains("baud") && j["baud"].is_number_integer()) {
        baudRate = j["baud"].get<int>();
    }

    if (j.contains("serialDevicePath") && j["serialDevicePath"].is_string()) {
        serialDevicePath = j["serialDevicePath"].get<std::string>();
    } else if (j.contains("port") && j["port"].is_string()) {
        serialDevicePath = j["port"].get<std::string>();
    }

    // "profile" seeds capability defaults; "capabilities" overrides individual flags.
    if (j.contains("profile") && j["profile"].is_string()) {
        capabilities = capsFromProfileString(j["profile"].get<std::string>());
    }

    if (j.contains("capabilities") && j["capabilities"].is_object()) {
        const ofJson& c = j["capabilities"];
        readBool(c, "supportsRealtimeStatusQuery", capabilities.supportsRealtimeStatusQuery);
        readBool(c, "supportsDollarSettings",      capabilities.supportsDollarSettings);
        readBool(c, "supportsGrblJog",             capabilities.supportsGrblJog);
        readBool(c, "supportsJogCancel",           capabilities.supportsJogCancel);
        readBool(c, "supportsM114Position",        capabilities.supportsM114Position);
        readBool(c, "supportsM115FirmwareInfo",    capabilities.supportsM115FirmwareInfo);
    }

    if (j.contains("envelope") && j["envelope"].is_object()) {
        const ofJson& e = j["envelope"];
        auto getf = [&](const char* key, float& out) {
            if (e.contains(key) && e[key].is_number()) out = jsonToFloat(e[key]);
        };
        getf("minX", envelope.minX); getf("minY", envelope.minY); getf("minZ", envelope.minZ);
        getf("maxX", envelope.maxX); getf("maxY", envelope.maxY); getf("maxZ", envelope.maxZ);
    }

    baudRate = std::max(9600, baudRate);
}

void MachinePrefs::save(const std::string& jsonPath, bool jsonPathIsAbsolute) const {
    const std::string resolved = resolveJsonPath(jsonPath, jsonPathIsAbsolute);
    ofFilePath::createEnclosingDirectory(ofFilePath::getEnclosingDirectory(resolved), false, true);

    ofJson j;
    j["baudRate"]         = baudRate;
    j["serialDevicePath"] = serialDevicePath;
    j["profile"]          = capsToProfileString(capabilities);

    ofJson caps;
    caps["supportsRealtimeStatusQuery"] = capabilities.supportsRealtimeStatusQuery;
    caps["supportsDollarSettings"]      = capabilities.supportsDollarSettings;
    caps["supportsGrblJog"]             = capabilities.supportsGrblJog;
    caps["supportsJogCancel"]           = capabilities.supportsJogCancel;
    caps["supportsM114Position"]        = capabilities.supportsM114Position;
    caps["supportsM115FirmwareInfo"]    = capabilities.supportsM115FirmwareInfo;
    j["capabilities"] = caps;

    ofJson env;
    env["minX"] = envelope.minX; env["minY"] = envelope.minY; env["minZ"] = envelope.minZ;
    env["maxX"] = envelope.maxX; env["maxY"] = envelope.maxY; env["maxZ"] = envelope.maxZ;
    j["envelope"] = env;

    if (!ofSavePrettyJson(resolved, j)) {
        ofLogError("grbl::MachinePrefs") << "failed to save " << resolved;
    }
}

} // namespace grbl
