# ofxGrbl

An openFrameworks addon for communicating with [GRBL](https://github.com/grbl/grbl) — the open-source G-code motion controller firmware for Arduino and compatible boards.

Works with any GRBL-driven machine: pen plotters, laser cutters, CNC routers, foam cutters, pick-and-place, and anything else that moves in X/Y/Z.

---

## Features

- **Queued G-code sender** — classic `ok`-handshake protocol, one line in flight at a time
- **Real-time commands** — feed hold (`!`), cycle start (`~`), jog cancel (`0x85`), status query (`?`) sent as raw bytes so GRBL's ISR picks them up immediately
- **Jog support** — `$J=` incremental and continuous jog with instant cancel
- **Queue pause / resume** — stop dispatching new lines without halting the machine mid-move; combine with feed hold for a full pause
- **Editor-line tracking** — tag each queued line with its source-file line number to drive a "follow the current command" highlight in a code editor UI
- **Settings round-trip** — `GrblSettings` parses the full `$$` block into typed fields and provides `formatLine()` helpers to write individual settings back
- **Simulation mode** — drain the queue with synthetic `ok` replies when no hardware is attached, for UI development and testing
- **Console ring buffer** — TX/RX log for display in a serial monitor window; status reports are kept in a separate slot so a 10 Hz poll loop can't drown meaningful output

---

## Requirements

- openFrameworks 0.12+
- No extra addons required — only `ofSerial` (bundled with OF)

> The old `ofxUI` / `ofxXmlSettings` dependency has been removed. UI and settings persistence are left to the calling application.

---

## Quick start

```cpp
#include "ofxGrbl.h"   // pulls in GrblSender + GrblSettings

class ofApp : public ofBaseApp {
    grbl::GrblSender sender;
    grbl::GrblSettings settings;
public:
    void setup() {
        // Connect — use the port your Arduino appears on
        sender.connectSerial("COM3", 115200);   // Windows
        // sender.connectSerial("/dev/ttyUSB0", 115200);  // Linux
        // sender.connectSerial("/dev/tty.usbmodem*", 115200);  // macOS

        // Or test without hardware:
        // sender.setSimulationMode(true);
    }

    void update() {
        sender.update();  // call every frame
    }

    void keyPressed(int key) {
        if (key == 'p') {
            // Queue a multi-line G-code block
            sender.enqueueGCodeBlock(
                "G21\n"          // millimetre mode
                "G90\n"          // absolute positioning
                "G0 X0 Y0\n"
                "G1 X100 Y0 F1000\n"
                "G1 X100 Y100\n"
                "G0 X0 Y0\n"
            );
        }
        if (key == ' ') sender.sendFeedHold();   // pause mid-move
        if (key == 'r') sender.sendCycleStart(); // resume
    }
};
```

---

## API overview

### `grbl::GrblSender`

| Method | Description |
|---|---|
| `connectSerial(port, baud)` | Open serial connection |
| `disconnectSerial()` | Close and clear queue |
| `setSimulationMode(bool)` | Bench mode — no hardware needed |
| `enqueueLine(line, editorLine)` | Queue one G-code line |
| `enqueueGCodeBlock(text)` | Queue a multi-line block (strips comments and blanks) |
| `sendImmediateLine(line)` | Bypass queue — urgent one-shot command |
| `clearQueue()` | Discard all pending lines |
| `setQueuePaused(bool)` | Stop / resume dispatching from the queue |
| `sendFeedHold()` | GRBL real-time `!` — smooth deceleration to a stop |
| `sendCycleStart()` | GRBL real-time `~` — resume after feed hold |
| `sendJogCancel()` | GRBL real-time `0x85` — abort `$J=` jog immediately |
| `sendRealtimeStatusQuery()` | Send `?` — result appears in `getLastStatusReport()` |
| `getLastStatusReport()` | Most recent `<state\|MPos:...>` string |
| `getStatusReportSeq()` | Increments each time a new status report arrives |
| `currentEditorLine()` | 0-based editor line of the currently executing command |
| `consoleLines()` | Ring buffer of TX/RX strings for a serial monitor UI |
| `update()` | Call every frame from `ofApp::update()` |

### `grbl::GrblSettings`

```cpp
grbl::GrblSettings s;
s.parseBlock(rawTextFromGrbl);  // feed the $$ reply

// Read typed fields
float maxX = s.maxTravelX;
bool  soft = s.softLimitsEnabled;

// Write a change back
sender.sendImmediateLine(grbl::GrblSettings::formatLine(130, 350.f)); // $130=350.000
```

`GrblSettings::parseBlock()` populates both the typed fields and `s.all` — a `std::map<int,std::string>` of every `$N=value` pair, so you can read settings that don't have a named field.

---

## Jogging

```cpp
// Incremental jog: 10 mm in +X at 3000 mm/min
sender.enqueueLine("$J=G91 G21 X10 F3000");

// Cancel an in-progress jog instantly (does not touch the G-code queue)
sender.sendJogCancel();
```

GRBL's `$J=` commands use a separate planner buffer so they don't interrupt queued motion jobs. Jog cancel sends the raw `0x85` byte, which GRBL handles at interrupt level.

---

## Configuring GRBL

Send `$$` to dump all settings:

```
$$
```

Example reply (GRBL 1.1):

```
$0=10    (step pulse, usec)
$20=0    (soft limits, bool)
$21=0    (hard limits, bool)
$110=3000.000  (x max rate, mm/min)
$111=3000.000  (y max rate, mm/min)
$130=400.000   (x max travel, mm)
$131=300.000   (y max travel, mm)
...
ok
```

Change a setting with:

```
$110=1500
```

Use `GrblSettings::parseBlock()` + `formatLine()` to do this programmatically — see example above.

---

## Steps-per-mm formula

```
stepsPerMm = (motorSteps × microSteps) / (beltPitchMm × pulleyTeeth)
```

Convenience helper:

```cpp
float spm = grbl::GrblSettings::stepsPerMm(200, 8, 2.0f, 20);
// 200-step motor, 1/8 microstep, GT2 belt, 20-tooth pulley → 40 steps/mm
sender.sendImmediateLine(grbl::GrblSettings::formatLine(100, spm)); // $100=40.000
```

---

## Console ring buffer

`GrblSender::consoleLines()` is a `std::deque<std::string>` that mirrors every TX (`> ...`) and RX (`< ...`) line up to `kMaxConsoleLines` (256). Wire it directly to an ImGui or ofxGui text list for a live serial monitor.

Status reports (`<Idle|MPos:...>`) are stored separately in `getLastStatusReport()` and do **not** appear in the console ring unless you set `sender.logStatusReports = true`. This prevents a 10 Hz poll loop from drowning meaningful output.

---

## Upgrading from the old ofxGrbl

The previous version depended on `ofxUI` and `ofxXmlSettings` and provided a self-contained UI widget. The new version is a pure communication layer — UI and settings persistence are the application's responsibility. If you need the old behaviour it is preserved on the `legacy` branch.

Key differences:

| Old | New |
|---|---|
| `ofxGrbl::sendMessage(line)` | `GrblSender::enqueueLine(line)` or `sendImmediateLine(line)` |
| `ofxGrbl::sendQueList` | `GrblSender::consoleLines()` |
| `GrblSettings` (extends `ofBaseApp`) | `grbl::GrblSettings` (plain struct, no OF dependency) |
| `ofxUI` panel | Bring your own ImGui / ofxGui widget |

---

## License

MIT — see [LICENSE](LICENSE).
