#pragma once

/// ofxGrbl — GRBL controller addon for openFrameworks
///
/// Include this single header to pull in everything:
///
///   #include "ofxGrbl.h"
///
///   grbl::GrblSender sender;
///   grbl::GrblSettings settings;
///   grbl::MachinePrefs prefs;
///   prefs.capabilities = grbl::MachineCapabilities::marlin(); // or grbl() / custom()
///   prefs.load();                                  // bin/data/settings/MachinePrefs.json
///   prefs.load("/path/to/custom.json", true);      // absolute path, any .json file
///   prefs.save("exports/machine.json", false);     // bin/data/exports/machine.json
///
/// See GrblSender.h and GrblSettings.h for full API documentation.

#include "GrblSender.h"
#include "GrblSettings.h"
#include "MachinePrefs.h"
#include "GCodeEnvelopeCheck.h"
