#include "ofApp.h"
#include <sstream>
#include <fstream>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(28);
    font_.load(OF_TTF_MONO, 12);

    sender.setSimulationMode(true);
}

void ofApp::update() {
    sender.update();

    if (sender.isConnected()) {
        float now = ofGetElapsedTimef();
        if (now - lastStatusTime_ > kStatusIntervalSec) {
            sender.sendRealtimeStatusQuery();
            lastStatusTime_ = now;
        }
        // Parse status if new
        uint32_t seq = sender.getStatusReportSeq();
        if (seq != lastStatusSeq_) {
            lastStatusSeq_ = seq;
            parseStatusReport(sender.getLastStatusReport());
        }
    }

    // Auto-clear printing flag when queue drains
    if (isPrinting_ && sender.pendingLines() == 0 && !sender.isWaitingAck()) {
        isPrinting_ = false;
    }
}

void ofApp::draw() {
    const float pad = 8.f;
    const float w   = ofGetWidth();
    const float h   = ofGetHeight();

    // Layout: left column = status + jog + settings, right column = console
    const float leftW  = 380.f;
    const float rightX = leftW + pad * 2;
    const float rightW = w - rightX - pad;

    drawStatus(pad, pad, leftW, 200);
    drawSettings(pad, 200 + pad * 3, leftW, h - 200 - pad * 4);
    drawConsole(rightX, pad, rightW, h - pad * 2);

    // Bottom hint bar
    ofSetColor(60);
    ofDrawRectangle(0, h - 22, w, 22);
    ofSetColor(130);
    font_.drawString(
        "C=connect  S=sim  L=load  SPACE=start/stop  H=hold  R=resume  P=pause  "
        "arrows=jog(+Shift×10)  G=read$$  X=alarm unlock",
        pad, h - 6);
}

// ---------------------------------------------------------------------------
// Key input
// ---------------------------------------------------------------------------

void ofApp::keyPressed(int key) {
    bool shift = ofGetKeyPressed(OF_KEY_SHIFT);
    float step = shift ? 10.f : 1.f;

    switch (key) {
    case 'c': case 'C': sender.connectSerial(PORT, BAUD);                  break;
    case 's': case 'S': sender.setSimulationMode(!sender.isSimulationMode()); break;
    case 'l': case 'L': {
        ofFileDialogResult r = ofSystemLoadDialog("Load G-code file");
        if (r.bSuccess) loadGCodeFile(r.getPath());
        break;
    }
    case ' ':
        if (isPrinting_) stopPrint(); else startPrint();
        break;
    case 'h': case 'H': sender.sendFeedHold();   break;
    case 'r': case 'R': sender.sendCycleStart();  break;
    case 'p': case 'P': sender.setQueuePaused(!sender.isQueuePaused()); break;
    case 'g': case 'G': sender.enqueueLine("$$");  break;
    case 'x': case 'X': sender.sendImmediateLine("$X"); break;

    case OF_KEY_RIGHT: sendJog( step,  0);    break;
    case OF_KEY_LEFT:  sendJog(-step,  0);    break;
    case OF_KEY_UP:    sendJog( 0,     step); break;
    case OF_KEY_DOWN:  sendJog( 0,    -step); break;
    case OF_KEY_PAGE_UP:   sendJog(0, 0,  step); break;
    case OF_KEY_PAGE_DOWN: sendJog(0, 0, -step); break;
    }
}

void ofApp::keyReleased(int key) {}

void ofApp::dragEvent(ofDragInfo info) {
    if (!info.files.empty()) loadGCodeFile(info.files[0]);
}

// ---------------------------------------------------------------------------
// G-code file
// ---------------------------------------------------------------------------

void ofApp::loadGCodeFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return;
    std::ostringstream ss;
    ss << f.rdbuf();
    loadedFile_ = ss.str();
}

void ofApp::startPrint() {
    if (loadedFile_.empty()) return;
    sender.enqueueGCodeBlock(loadedFile_);
    isPrinting_ = true;
}

void ofApp::stopPrint() {
    sender.sendFeedHold();
    sender.clearQueue();
    isPrinting_ = false;
    sender.sendImmediateLine("G0 Z5");  // safe Z lift
}

// ---------------------------------------------------------------------------
// Jog
// ---------------------------------------------------------------------------

void ofApp::sendJog(float dx, float dy, float dz) {
    if (!sender.isConnected()) return;
    char buf[64];
    std::snprintf(buf, sizeof(buf),
                  "$J=G21 G91 X%.3f Y%.3f Z%.3f F%.0f",
                  dx, dy, dz, jogFeedMmMin_);
    sender.enqueueLine(buf);
}

// ---------------------------------------------------------------------------
// Status parsing
// ---------------------------------------------------------------------------

void ofApp::parseStatusReport(const std::string& report) {
    // Format: <State|MPos:x,y,z|FS:feed,speed|...>
    auto tag = [&](const std::string& key) -> std::string {
        size_t pos = report.find(key + ":");
        if (pos == std::string::npos) return "";
        pos += key.size() + 1;
        size_t end = report.find_first_of("|>", pos);
        return report.substr(pos, end - pos);
    };

    // State is between '<' and first '|'
    size_t stBar = report.find('|');
    if (stBar != std::string::npos && report.size() > 1)
        grblState_ = report.substr(1, stBar - 1);

    std::string mpos = tag("MPos");
    if (!mpos.empty()) {
        std::istringstream ss(mpos);
        char comma;
        ss >> mposX_ >> comma >> mposY_ >> comma >> mposZ_;
    }
}

// ---------------------------------------------------------------------------
// Drawing panels
// ---------------------------------------------------------------------------

void ofApp::drawPanel(float x, float y, float w, float h, const std::string& title) {
    ofSetColor(40);
    ofDrawRectangle(x, y, w, h);
    ofSetColor(70);
    ofDrawRectangle(x, y, w, 20);
    ofSetColor(200);
    font_.drawString(title, x + 6, y + 14);
}

void ofApp::drawStatus(float x, float y, float w, float h) {
    drawPanel(x, y, w, h, "Status");
    float lh = 16.f;
    float ty = y + 22;
    auto line = [&](const std::string& label, const std::string& val, ofColor vc = ofColor(220)) {
        ofSetColor(120); font_.drawString(label, x + 6, ty);
        ofSetColor(vc);  font_.drawString(val, x + 100, ty);
        ty += lh;
    };

    std::string connStr = sender.isSimulationMode() ? "SIMULATION"
                        : sender.isUsbConnected()   ? "CONNECTED"
                                                    : "DISCONNECTED";
    ofColor connCol = sender.isSimulationMode() ? ofColor(255,200,0)
                    : sender.isUsbConnected()   ? ofColor(0,220,80)
                                                : ofColor(220,60,60);

    line("Connection", connStr, connCol);
    line("GRBL state", grblState_.empty() ? "-" : grblState_,
         grblState_ == "Idle" ? ofColor(0,220,80) :
         grblState_ == "Run"  ? ofColor(80,180,255) :
         grblState_ == "Hold" ? ofColor(255,180,0) :
         grblState_ == "Alarm"? ofColor(255,60,60) : ofColor(200));
    line("MPos X",  ofToString(mposX_, 3) + " mm");
    line("MPos Y",  ofToString(mposY_, 3) + " mm");
    line("MPos Z",  ofToString(mposZ_, 3) + " mm");
    line("Queue",   std::to_string(sender.pendingLines()) + " lines"
                    + (sender.isQueuePaused() ? "  [PAUSED]" : "")
                    + (isPrinting_            ? "  [PRINTING]" : ""),
         sender.isQueuePaused() ? ofColor(255,180,0) : ofColor(220));

    ty += lh * 0.5f;
    ofSetColor(100);
    font_.drawString("File: " + (loadedFile_.empty() ? "(none loaded)" :
                     std::to_string(std::count(loadedFile_.begin(), loadedFile_.end(), '\n'))
                     + " lines"), x + 6, ty);
}

void ofApp::drawSettings(float x, float y, float w, float h) {
    drawPanel(x, y, w, h, "Machine settings  (press G to read from device)");
    if (!settingsLoaded) {
        ofSetColor(100);
        font_.drawString("Not loaded yet. Press G to send $$.", x + 8, y + 36);
        // Try to parse from console in case $$ was just received
        std::string combined;
        for (const auto& l : sender.consoleLines()) combined += l + "\n";
        if (settings.parseBlock(combined) > 0) settingsLoaded = true;
        return;
    }
    float lh = 15.f;
    float ty = y + 22;
    auto row = [&](const std::string& label, float val, const std::string& unit = "mm") {
        ofSetColor(130); font_.drawString(label, x + 6,   ty);
        ofSetColor(220); font_.drawString(ofToString(val, 3), x + 160, ty);
        ofSetColor(90);  font_.drawString(unit,  x + 240, ty);
        ty += lh;
    };
    auto rowB = [&](const std::string& label, bool val) {
        ofSetColor(130); font_.drawString(label, x + 6, ty);
        ofSetColor(val ? ofColor(0,220,80) : ofColor(180,60,60));
        font_.drawString(val ? "ON" : "OFF", x + 160, ty);
        ty += lh;
    };
    rowB("Soft limits ($20)",  settings.softLimitsEnabled);
    rowB("Hard limits ($21)",  settings.hardLimitsEnabled);
    rowB("Homing ($22)",       settings.homingEnabled);
    ty += lh * 0.3f;
    row("Max rate X ($110)", settings.maxRateX, "mm/min");
    row("Max rate Y ($111)", settings.maxRateY, "mm/min");
    row("Max rate Z ($112)", settings.maxRateZ, "mm/min");
    ty += lh * 0.3f;
    row("Max travel X ($130)", settings.maxTravelX, "mm");
    row("Max travel Y ($131)", settings.maxTravelY, "mm");
    row("Max travel Z ($132)", settings.maxTravelZ, "mm");
    ty += lh * 0.3f;
    row("Steps/mm X ($100)", settings.stepsPerMmX, "st/mm");
    row("Steps/mm Y ($101)", settings.stepsPerMmY, "st/mm");
    row("Steps/mm Z ($102)", settings.stepsPerMmZ, "st/mm");
}

void ofApp::drawConsole(float x, float y, float w, float h) {
    drawPanel(x, y, w, h, "Console  (TX / RX)");
    const float lh   = 15.f;
    const float pad  = 6.f;
    int maxLines = (int)((h - 24) / lh);
    const auto& lines = sender.consoleLines();
    int start = (int)lines.size() > maxLines ? (int)lines.size() - maxLines : 0;
    float ty = y + 22;
    for (int i = start; i < (int)lines.size(); ++i) {
        const std::string& l = lines[i];
        ofColor c(170);
        if (l.size() >= 2 && l[0] == '>' && l[1] == '>')  c = ofColor(255, 220, 80);
        else if (l.find("[ERR]")     != std::string::npos) c = ofColor(255, 80,  80);
        else if (l.find("[Timeout]") != std::string::npos) c = ofColor(255, 140, 40);
        else if (l[0] == '[')                              c = ofColor(120, 190, 255);
        ofSetColor(c);
        font_.drawString(l, x + pad, ty);
        ty += lh;
    }
}
