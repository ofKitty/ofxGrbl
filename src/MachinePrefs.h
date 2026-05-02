#pragma once

#include <string>

namespace grbl {

/// Software work envelope in machine coordinates (mm) — not GRBL $130-132, but
/// used by the app UI to clamp jogs and validate outgoing G-code blocks.
struct Envelope {
    float minX = 0.f, minY = 0.f, minZ = -10.f;
    float maxX = 420.f, maxY = 297.f, maxZ = 40.f;

    void clampXYZ(float& x, float& y, float& z) const;
};

/// Feature set for a specific machine. Default-constructed = full GRBL feature set.
///
/// Use the static factory methods as a starting point, then override individual
/// flags for machines that differ from a standard profile:
///
///   auto caps = grbl::MachineCapabilities::marlin();
///   caps.supportsM115FirmwareInfo = false; // this board doesn't implement M115
struct MachineCapabilities {
    bool supportsRealtimeStatusQuery = true;  ///< GRBL `?` status reports.
    bool supportsDollarSettings      = true;  ///< GRBL `$$` settings dump.
    bool supportsGrblJog             = true;  ///< GRBL `$J=` jog commands.
    bool supportsJogCancel           = true;  ///< GRBL realtime 0x85 jog cancel.
    bool supportsM114Position        = false; ///< Marlin-style M114 position query.
    bool supportsM115FirmwareInfo    = false; ///< Marlin-style M115 firmware info.

    /// Full GRBL feature set (default).
    static MachineCapabilities grbl();

    /// Marlin / FDM printers: M114/M115, no GRBL $ commands or realtime bytes.
    static MachineCapabilities marlin();

    /// All capabilities off — opt in to exactly what your machine supports.
    static MachineCapabilities custom();
};

/// Connection + workspace envelope + capabilities for the machine the app talks to.
///
/// `load` / `save` read and write JSON (pretty-printed). With an empty `jsonPath`,
/// uses `defaultRelativePath()` under `bin/data/`. Otherwise `jsonPath` may be any
/// `.json` file; `jsonPathIsAbsolute` selects a filesystem path vs. data-relative.
///
/// The JSON `"profile"` key (`"grbl"`, `"marlin"`, `"custom"`) seeds capability
/// defaults; any `"capabilities"` overrides are applied on top.
struct MachinePrefs {
    int                 baudRate = 115200;
    std::string         serialDevicePath;
    Envelope            envelope;
    MachineCapabilities capabilities;  // default = grbl

    /// Path relative to `bin/data/` used when `load`/`save` get an empty `jsonPath`.
    static const char* defaultRelativePath() { return "settings/MachinePrefs.json"; }

    /// @param jsonPath          Empty → `defaultRelativePath()` under bin/data.
    ///                          Non-empty → any `.json` file path.
    /// @param jsonPathIsAbsolute When true, `jsonPath` is a full filesystem path.
    void load(const std::string& jsonPath = "", bool jsonPathIsAbsolute = false);

    void save(const std::string& jsonPath = "", bool jsonPathIsAbsolute = false) const;
};

} // namespace grbl
