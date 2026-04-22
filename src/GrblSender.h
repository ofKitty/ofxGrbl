#pragma once

#include "ofSerial.h"
#include <cstdint>
#include <deque>
#include <string>

namespace grbl {

/// Queued G-code sender for GRBL over USB serial.
///
/// Protocol: classic ok-handshake — one line out, wait for "ok" back, repeat.
/// Real-time commands (feed hold, cycle start, jog cancel, status query) bypass
/// the queue and are written as raw bytes so GRBL's ISR picks them up instantly.
///
/// Usage:
///   grbl::GrblSender sender;
///   sender.connectSerial("/dev/ttyUSB0", 115200);
///   sender.enqueueGCodeBlock("G90\nG0 X10 Y10\n");
///   // call sender.update() every frame from ofApp::update()
class GrblSender {
public:
    static constexpr float kSendTimeoutSec  = 5.f;
    static constexpr int   kMaxConsoleLines = 256;

    // -----------------------------------------------------------------------
    // Connection
    // -----------------------------------------------------------------------

    bool connectSerial(const std::string& devicePath, int baud);
    void disconnectSerial();

    /// True when USB is open, or simulation mode is active.
    bool isConnected()    const { return connected_ || simulationMode_; }
    bool isUsbConnected() const { return connected_; }

    /// Bench / UI testing: drain the queue locally with synthetic "ok" replies.
    /// No hardware required.
    void setSimulationMode(bool enabled);
    bool isSimulationMode() const { return simulationMode_; }

    // -----------------------------------------------------------------------
    // Real-time commands (single raw bytes, bypass ok-handshake)
    // -----------------------------------------------------------------------

    /// Send `?` status query. Does not consume a queue slot.
    /// By default the `?` TX and `<...>` RX are NOT echoed to the console ring
    /// (at 10 Hz they drown anything useful). Set logStatusReports = true to
    /// see the raw heartbeat for debugging.
    void sendRealtimeStatusQuery();

    /// Send `!` feed hold. Motion decelerates smoothly inside the current line.
    void sendFeedHold();

    /// Send `~` cycle start / resume after a feed hold.
    void sendCycleStart();

    /// Send 0x85 jog cancel. Stops any in-progress $J= jog moves instantly.
    void sendJogCancel();

    /// Most recent `<...>` status report (raw, including outer `<>`).
    /// Empty string if none received yet.
    std::string getLastStatusReport() const { return lastStatusReport_; }

    /// Increments every time a new status report arrives. Use to detect
    /// "new data since I last looked" without string-comparing the payload.
    uint32_t getStatusReportSeq() const { return statusReportSeq_; }

    /// Mirror status query traffic into the console ring (default: false).
    bool logStatusReports = false;

    // -----------------------------------------------------------------------
    // Queue control
    // -----------------------------------------------------------------------

    /// Pause / resume local queue dispatch. Any line already in-flight to GRBL
    /// runs to completion. Combine with sendFeedHold() to also halt mid-line.
    void setQueuePaused(bool paused);
    bool isQueuePaused() const { return queuePaused_; }

    /// Enqueue one stripped G-code line (no CR/LF needed).
    /// editorLine: optional 0-based source-editor line index for UI highlighting
    /// during a print. Pass -1 if not applicable.
    void enqueueLine(const std::string& line, int editorLine = -1);

    /// Split a multi-line G-code block into individual lines, strip comments
    /// and blank lines, and enqueue with editor-line tracking.
    void enqueueGCodeBlock(const std::string& text);

    void clearQueue();

    /// Bypass the queue — use for urgent one-shot commands (e.g. pen-up on stop).
    void sendImmediateLine(const std::string& line);

    size_t pendingLines()  const { return sendQueue_.size(); }
    bool   isWaitingAck()  const { return waitingForOk_; }

    // -----------------------------------------------------------------------
    // Editor line tracking
    // -----------------------------------------------------------------------

    /// 0-based editor line of the most recently dispatched queue entry,
    /// or -1 if untagged. Drive a code-editor highlight from this during print.
    int currentEditorLine() const { return currentEditorLine_; }

    // -----------------------------------------------------------------------
    // Console ring buffer (TX/RX log for the UI)
    // -----------------------------------------------------------------------

    std::deque<std::string>&       consoleLines()       { return consoleLines_; }
    const std::deque<std::string>& consoleLines() const { return consoleLines_; }
    void clearConsole();

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /// Call every frame from ofApp::update().
    void update();

    /// Drain any startup noise GRBL sends on connect (Grbl version banner, etc).
    void flushStartupNoise();

private:
    void processSerialIO();
    void sendNextLine();
    void pushConsole(const std::string& line);
    static bool lineMeansOk(const std::string& line);

    ofSerial serial_;
    bool connected_      = false;
    bool simulationMode_ = false;

    std::deque<std::string> sendQueue_;
    std::deque<int>         sendQueueEditorLines_;
    int                     currentEditorLine_ = -1;
    bool  waitingForOk_  = false;
    bool  queuePaused_   = false;
    float lastSendTime_  = 0.f;

    std::string             rxBuffer_;
    std::deque<std::string> consoleLines_;

    // Status reports are kept in their own slot so a 10 Hz poll loop
    // can't drown the user-visible console.
    std::string lastStatusReport_;
    uint32_t    statusReportSeq_ = 0;
};

} // namespace grbl
