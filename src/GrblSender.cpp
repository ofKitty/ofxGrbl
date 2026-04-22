#include "GrblSender.h"
#include "ofMain.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace grbl {

// ---------------------------------------------------------------------------
// Connection
// ---------------------------------------------------------------------------

bool GrblSender::connectSerial(const std::string& devicePath, int baud) {
    simulationMode_ = false;
    disconnectSerial();
    if (!serial_.setup(devicePath, baud)) {
        pushConsole("[Error] Failed to connect to " + devicePath);
        return false;
    }
    connected_ = true;
    pushConsole("[Connected] " + devicePath + " @ " + std::to_string(baud));
    flushStartupNoise();
    return true;
}

void GrblSender::disconnectSerial() {
    const bool hadUsb = connected_;
    if (connected_) {
        serial_.close();
        connected_ = false;
    }
    sendQueue_.clear();
    sendQueueEditorLines_.clear();
    waitingForOk_ = false;
    rxBuffer_.clear();
    if (hadUsb) pushConsole("[Disconnected]");
}

void GrblSender::setSimulationMode(bool enabled) {
    if (simulationMode_ == enabled) return;
    sendQueue_.clear();
    sendQueueEditorLines_.clear();
    waitingForOk_ = false;
    rxBuffer_.clear();
    const bool hadUsb = connected_;
    if (connected_) { serial_.close(); connected_ = false; }
    simulationMode_ = enabled;
    if (hadUsb)  pushConsole("[Disconnected]");
    if (enabled) pushConsole("[Simulation] no USB — synthetic ok responses active");
}

// ---------------------------------------------------------------------------
// Real-time commands
// ---------------------------------------------------------------------------

void GrblSender::sendRealtimeStatusQuery() {
    if (simulationMode_) {
        lastStatusReport_ = "<Sim|Idle|MPos:0.000,0.000,0.000|FS:0,0>";
        ++statusReportSeq_;
        if (logStatusReports) {
            pushConsole(">> ?");
            pushConsole("< " + lastStatusReport_);
        }
        return;
    }
    if (!connected_) return;
    unsigned char q = '?';
    serial_.writeBytes(&q, 1);
    if (logStatusReports) pushConsole(">> ?");
}

void GrblSender::sendFeedHold() {
    if (simulationMode_) { pushConsole(">> ! (feed hold)");    return; }
    if (!connected_)      return;
    unsigned char c = '!';
    serial_.writeBytes(&c, 1);
    pushConsole(">> ! (feed hold)");
}

void GrblSender::sendCycleStart() {
    if (simulationMode_) { pushConsole(">> ~ (cycle start)");  return; }
    if (!connected_)      return;
    unsigned char c = '~';
    serial_.writeBytes(&c, 1);
    pushConsole(">> ~ (cycle start)");
}

void GrblSender::sendJogCancel() {
    if (simulationMode_) { pushConsole(">> 0x85 (jog cancel)"); return; }
    if (!connected_)      return;
    unsigned char c = 0x85;
    serial_.writeBytes(&c, 1);
    pushConsole(">> 0x85 (jog cancel)");
}

// ---------------------------------------------------------------------------
// Queue
// ---------------------------------------------------------------------------

void GrblSender::setQueuePaused(bool paused) {
    if (queuePaused_ == paused) return;
    queuePaused_ = paused;
    pushConsole(paused ? "[Queue paused]" : "[Queue resumed]");
}

void GrblSender::enqueueLine(const std::string& line, int editorLine) {
    if (line.empty()) return;
    sendQueue_.push_back(line);
    sendQueueEditorLines_.push_back(editorLine);
}

void GrblSender::enqueueGCodeBlock(const std::string& text) {
    std::istringstream stream(text);
    std::string line;
    int count   = 0;
    int srcLine = 0;
    while (std::getline(stream, line)) {
        int thisLine = srcLine++;
        // Strip inline comments
        size_t semi = line.find(';');
        if (semi != std::string::npos) line.resize(semi);
        // Strip trailing whitespace / CR
        while (!line.empty() && (line.back() == ' ' || line.back() == '\r' || line.back() == '\t'))
            line.pop_back();
        if (line.empty()) continue;
        sendQueue_.push_back(line);
        sendQueueEditorLines_.push_back(thisLine);
        ++count;
    }
    pushConsole("[Queued] " + std::to_string(count) + " commands");
}

void GrblSender::clearQueue() {
    sendQueue_.clear();
    sendQueueEditorLines_.clear();
    currentEditorLine_ = -1;
}

void GrblSender::sendImmediateLine(const std::string& line) {
    if (line.empty() || (!connected_ && !simulationMode_)) return;
    pushConsole(">> " + line);
    if (simulationMode_) {
        pushConsole("< ok");
        waitingForOk_ = false;
        return;
    }
    std::string toSend = line + "\n";
    serial_.writeBytes((unsigned char*)toSend.c_str(), (int)toSend.size());
    waitingForOk_ = true;
    lastSendTime_ = ofGetElapsedTimef();
}

// ---------------------------------------------------------------------------
// Console
// ---------------------------------------------------------------------------

void GrblSender::clearConsole() {
    consoleLines_.clear();
}

void GrblSender::pushConsole(const std::string& line) {
    consoleLines_.push_back(line);
    while ((int)consoleLines_.size() > kMaxConsoleLines)
        consoleLines_.pop_front();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void GrblSender::flushStartupNoise() {
    if (!connected_) return;
    ofSleepMillis(80);
    int safety = 0;
    while (serial_.available() > 0 && safety++ < 4096) {
        int n = std::min(serial_.available(), 4096);
        std::vector<unsigned char> buf((size_t)n);
        int got = serial_.readBytes(buf.data(), n);
        for (int i = 0; i < got; ++i) {
            char c = (char)buf[i];
            if (c == '\n' || c == '\r') {
                if (!rxBuffer_.empty()) {
                    pushConsole("< " + rxBuffer_);
                    rxBuffer_.clear();
                }
            } else {
                rxBuffer_ += c;
            }
        }
    }
    if (!rxBuffer_.empty()) {
        pushConsole("< " + rxBuffer_);
        rxBuffer_.clear();
    }
}

void GrblSender::update() {
    if (simulationMode_) {
        if (!queuePaused_) {
            int budget = 256;
            while (!sendQueue_.empty() && budget-- > 0) {
                std::string line = sendQueue_.front();
                sendQueue_.pop_front();
                if (!sendQueueEditorLines_.empty()) {
                    currentEditorLine_ = sendQueueEditorLines_.front();
                    sendQueueEditorLines_.pop_front();
                }
                pushConsole("> " + line);
                pushConsole("< ok");
            }
            waitingForOk_ = false;
        }
        return;
    }

    if (!connected_) return;
    processSerialIO();

    if (!queuePaused_ && !waitingForOk_ && !sendQueue_.empty())
        sendNextLine();

    if (waitingForOk_) {
        if (ofGetElapsedTimef() - lastSendTime_ > kSendTimeoutSec) {
            pushConsole("[Timeout] No ok received — clearing wait (check wiring/firmware)");
            waitingForOk_ = false;
        }
    }
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

bool GrblSender::lineMeansOk(const std::string& line) {
    size_t s = line.find_first_not_of(" \t\r\n");
    if (s == std::string::npos) return false;
    std::string t = line.substr(s);
    if (t.size() < 2 || t[0] != 'o' || t[1] != 'k') return false;
    return t.size() == 2 || std::isspace(static_cast<unsigned char>(t[2])) != 0;
}

void GrblSender::processSerialIO() {
    int avail = serial_.available();
    if (avail <= 0) return;
    int toRead = std::min(avail, 4096);
    std::vector<unsigned char> buf((size_t)toRead);
    int bytesRead = serial_.readBytes(buf.data(), toRead);
    for (int i = 0; i < bytesRead; ++i) {
        char c = (char)buf[i];
        if (c == '\n' || c == '\r') {
            if (!rxBuffer_.empty()) {
                std::string line = rxBuffer_;
                rxBuffer_.clear();

                // Real-time status — parked in its own slot, not the console.
                // Opt in via logStatusReports if you need to see the heartbeat.
                if (!line.empty() && line[0] == '<') {
                    lastStatusReport_ = line;
                    ++statusReportSeq_;
                    if (logStatusReports) pushConsole("< " + line);
                    continue;
                }

                if (lineMeansOk(line)) waitingForOk_ = false;
                if (line.find("error") != std::string::npos ||
                    line.find("ALARM") != std::string::npos) {
                    pushConsole("[ERR] " + line);
                    waitingForOk_ = false;
                } else {
                    pushConsole("< " + line);
                }
                while ((int)consoleLines_.size() > kMaxConsoleLines)
                    consoleLines_.pop_front();
            }
        } else {
            rxBuffer_ += c;
        }
    }
}

void GrblSender::sendNextLine() {
    if (sendQueue_.empty()) return;
    std::string line = sendQueue_.front();
    sendQueue_.pop_front();
    if (!sendQueueEditorLines_.empty()) {
        currentEditorLine_ = sendQueueEditorLines_.front();
        sendQueueEditorLines_.pop_front();
    }
    std::string toSend = line + "\n";
    serial_.writeBytes((unsigned char*)toSend.c_str(), (int)toSend.size());
    waitingForOk_ = true;
    lastSendTime_ = ofGetElapsedTimef();
    pushConsole("> " + line);
}

} // namespace grbl
