#include "ofApp.h"

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(30);
    font.load(OF_TTF_MONO, 13);

    // Start in simulation mode so the example works without hardware.
    // Press 'S' to toggle, 'C' to connect to real hardware.
    sender.setSimulationMode(true);
}

void ofApp::update() {
    sender.update();

    // Poll GRBL status at ~10 Hz
    if (sender.isConnected()) {
        float now = ofGetElapsedTimef();
        if (now - lastStatusTime_ > kStatusIntervalSec) {
            sender.sendRealtimeStatusQuery();
            lastStatusTime_ = now;
        }
    }
}

void ofApp::draw() {
    const float pad  = 14.f;
    const float lh   = 16.f;
    float y = pad + lh;

    auto text = [&](const std::string& s, ofColor c = ofColor(220)) {
        ofSetColor(c);
        font.drawString(s, pad, y);
        y += lh;
    };

    // --- status bar ---
    std::string connStr = sender.isSimulationMode() ? "SIMULATION"
                        : sender.isUsbConnected()   ? std::string("CONNECTED  ") + PORT
                                                    : "DISCONNECTED";
    text("ofxGrbl  |  " + connStr,
         sender.isSimulationMode() ? ofColor(255, 200, 0)
         : sender.isUsbConnected() ? ofColor(0, 220, 80)
                                   : ofColor(220, 60, 60));

    text("Status:  " + (sender.getLastStatusReport().empty()
                        ? "(none yet)"
                        : sender.getLastStatusReport()),
         ofColor(120, 200, 255));

    text("Queue:   " + std::to_string(sender.pendingLines()) + " lines pending"
         + (sender.isQueuePaused() ? "  [PAUSED]" : ""),
         ofColor(180));

    y += lh * 0.5f;
    text("Keys:  C=connect  S=simulation  P=print job  H=feed hold  R=resume  Q=clear  ?=status",
         ofColor(100));

    // --- console ring ---
    y += lh * 0.5f;
    text("--- console ---", ofColor(80));
    const auto& lines = sender.consoleLines();
    // Show most recent lines that fit in the window
    int maxLines = (int)((ofGetHeight() - y - pad) / lh);
    int start    = (int)lines.size() > maxLines
                   ? (int)lines.size() - maxLines : 0;
    for (int i = start; i < (int)lines.size(); ++i) {
        const std::string& l = lines[i];
        ofColor c(180);
        if (l.size() >= 2 && l[0] == '>' && l[1] == '>') c = ofColor(255, 220, 80);
        else if (l.find("[ERR]") != std::string::npos)     c = ofColor(255, 80,  80);
        else if (l.find("[Timeout]") != std::string::npos) c = ofColor(255, 140, 40);
        else if (l.find("[") == 0)                         c = ofColor(140, 200, 255);
        text(l, c);
    }
}

void ofApp::keyPressed(int key) {
    switch (key) {
    case 'c': case 'C':
        sender.connectSerial(PORT, BAUD);
        break;
    case 's': case 'S':
        sender.setSimulationMode(!sender.isSimulationMode());
        break;
    case 'p': case 'P':
        sender.enqueueGCodeBlock(
            "G21\n"                      // mm mode
            "G90\n"                      // absolute
            "G0 X0 Y0\n"
            "G1 X50 Y0 F1000\n"
            "G1 X50 Y50\n"
            "G1 X0  Y50\n"
            "G1 X0  Y0\n"
            "G0 X0 Y0\n"
        );
        break;
    case 'h': case 'H':
        sender.sendFeedHold();
        break;
    case 'r': case 'R':
        sender.sendCycleStart();
        break;
    case 'q': case 'Q':
        sender.clearQueue();
        break;
    case '?':
        sender.sendRealtimeStatusQuery();
        break;
    }
}
