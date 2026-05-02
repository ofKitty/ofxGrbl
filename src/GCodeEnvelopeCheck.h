#pragma once

#include "MachinePrefs.h"
#include <optional>
#include <string>

namespace grbl {

/// Returns a human-readable error if any parsed move end-point leaves the
/// [min,max] software envelope; otherwise `std::nullopt` (G-code is OK to send).
///
/// Handles common G0/G1 (and G2/G3 end points) in G21 mm; G90 absolute vs G91
/// incremental. Comments `;` and `()` and blank lines are ignored.
std::optional<std::string> checkGCodeBlockAgainstEnvelope(
    const std::string& gcode, const Envelope& env, float startX, float startY, float startZ
);

} // namespace grbl
