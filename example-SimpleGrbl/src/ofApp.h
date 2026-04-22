#pragma once

#include "ofMain.h"
#include "ofxGrbl.h"

/// example-SimpleGrbl
/// Demonstrates the minimal ofxGrbl workflow:
///   1. Connect to GRBL over serial (or run in simulation mode)
///   2. Queue a G-code job
///   3. Show live status and the console ring buffer
///
/// Key bindings:
///   C  — connect (edit PORT below to match your device)
///   S  — toggle simulation mode (no hardware required)
///   P  — queue a small test job
///   H  — send feed hold  (pause mid-move)
///   R  — send cycle start (resume)
///   Q  — clear the queue
///   ?  — request a status report
class ofApp : public ofBaseApp {
public:
    void setup()  override;
    void update() override;
    void draw()   override;
    void keyPressed(int key) override;

private:
    grbl::GrblSender   sender;
    grbl::GrblSettings settings;

    // Edit to match your machine's serial port:
    //   Windows:  "COM3"
    //   Linux:    "/dev/ttyUSB0"
    //   macOS:    "/dev/tty.usbmodem14201"
    static constexpr const char* PORT = "COM3";
    static constexpr int         BAUD = 115200;

    ofTrueTypeFont font;

    static constexpr float kStatusIntervalSec = 0.1f;
    float lastStatusTime_ = 0.f;
};
