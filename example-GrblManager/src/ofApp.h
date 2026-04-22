#pragma once

#include "ofMain.h"
#include "ofxGrbl.h"
#include <string>

/// example-GrblManager
/// Full-featured ofxGrbl demo:
///   - Serial connect / disconnect
///   - Simulation mode for bench testing
///   - Load and send a G-code file
///   - Feed hold / cycle start
///   - Queue pause / resume
///   - Incremental jog (arrow keys) and continuous jog (hold Shift)
///   - GRBL settings round-trip: read $$ and write individual changes
///   - Live position display (MPos from status report)
///   - Console ring buffer view
///
/// Key bindings:
///   C          — connect to hardware (see PORT)
///   S          — toggle simulation mode
///   L          — load G-code file via dialog
///   SPACE      — start / stop print
///   H          — feed hold
///   R          — cycle start (resume)
///   P          — pause / resume local queue
///   Arrow keys — jog 1 mm (hold Shift for 10 mm)
///   G          — send $$ to read machine settings
///   X          — send $X (kill alarm lock)
class ofApp : public ofBaseApp {
public:
    void setup()    override;
    void update()   override;
    void draw()     override;
    void keyPressed(int key)  override;
    void keyReleased(int key) override;
    void dragEvent(ofDragInfo info) override;

private:
    grbl::GrblSender   sender;
    grbl::GrblSettings settings;
    bool settingsLoaded = false;

    // Edit to match your machine
    static constexpr const char* PORT = "COM3";
    static constexpr int         BAUD = 115200;

    // Status polling
    static constexpr float kStatusIntervalSec = 0.1f;
    float lastStatusTime_ = 0.f;

    // Live position parsed from status report
    float mposX_ = 0.f, mposY_ = 0.f, mposZ_ = 0.f;
    std::string grblState_;
    uint32_t lastStatusSeq_ = 0;
    void parseStatusReport(const std::string& report);

    // G-code job
    std::string loadedFile_;
    bool        isPrinting_ = false;
    void loadGCodeFile(const std::string& path);
    void startPrint();
    void stopPrint();

    // Jog
    float jogStepMm_  = 1.f;
    float jogFeedMmMin_ = 3000.f;
    void  sendJog(float dx, float dy, float dz = 0.f);

    // Drawing helpers
    ofTrueTypeFont font_;
    void drawPanel(float x, float y, float w, float h, const std::string& title);
    void drawConsole(float x, float y, float w, float h);
    void drawStatus(float x, float y, float w, float h);
    void drawSettings(float x, float y, float w, float h);
};
