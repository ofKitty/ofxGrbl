# ofxGrbl
[Beta] This is an add-on for controlling [Grbl](https://github.com/grbl/grbl) (CNC control firmware for Arduino) from openFrameworks.

[![ofxGrbl: Grbl (CNC control firmware for Arduino) with openFrameworks demo](http://img.youtube.com/vi/3CR-sZpXvfI/0.jpg)](http://www.youtube.com/watch?v=3CR-sZpXvfI "ofxGrbl: Grbl (CNC control firmware for Arduino) with openFrameworks demo")

## Requirements

### Software

* **ofxGrbl** (this add-on)<br />
[https://github.com/TsubokuLab/ofxGrbl](https://github.com/TsubokuLab/ofxGrbl)

* **Grbl** (Arduino sketch / firmware)<br />
[https://github.com/grbl/grbl](https://github.com/grbl/grbl)

### Hardware

#### Electronics

| Item | Price (reference) | Notes | Where to buy |
| ------------- | ------------- | ------------- | ------------- |
| Arduino UNO | ~435 JPY each | Clones are fine | [http://amzn.to/2oNAEX2](http://amzn.to/2oNAEX2) |
| CNC shield | ~225 JPY each | Main shield for this setup; very useful | [http://amzn.to/2nLtFyg](http://amzn.to/2nLtFyg) |
| NEMA 17 stepper motor | $16.49 each | 200-step motor, DC 12–24 V / 1.7 A | [http://bit.ly/2nLyXde](http://bit.ly/2nLyXde) |
| A4988 (stepper driver) | ~1000 JPY for 5 | Burns out fairly often from overcurrent; keep spares | [http://amzn.to/2oNQD7r](http://amzn.to/2oNQD7r) |
| Endstop / limit switch | $0.44 each | Needed for homing; prevents damage if the axis over-travels. Skip if you do not use limits | [http://bit.ly/2opXI1Q](http://bit.ly/2opXI1Q) |
| 12–24 V 8 A AC adapter | ~2100 JPY each | ~1.7 A per motor; 8 A is a reasonable target. Easy to kill with a short—keep extras | [http://amzn.to/2oqFGN9](http://amzn.to/2oqFGN9) |
| Emergency-stop pushbutton | ~788 JPY each | Stops motion when the motor loads up or the board is in trouble. Very handy | [http://amzn.to/2oqHN3x](http://amzn.to/2oqHN3x) |

Bundled kits with Arduino + CNC shield + four A4988 drivers are also available:

* Arduino + CNC shield + A4988 ×4 — ~2630 JPY<br />
[http://amzn.to/2nLkYUN](http://amzn.to/2nLkYUN)

#### Rails and mechanics

A bit pricey, but OpenBuilds **V-Slot** extrusion is a solid choice.

* OpenBuilds part store<br />
[http://openbuildspartstore.com](http://openbuildspartstore.com)

Open-source hardware designs mean you can 3D-print related parts.

* Prefer the official store above for quality. Most parts also appear on AliExpress if you want lower cost:<br />
[https://www.aliexpress.com/store/123598](https://www.aliexpress.com/store/123598)

| Item | Price (reference) | Notes | Where to buy |
| ------------- | ------------- | ------------- | ------------- |
| V-Slot® linear rail | ~$6 / m | V-groove extrusion; max length per stick was around 1.5 m | 20 mm × 20 mm [http://bit.ly/2qD2H0e](http://bit.ly/2qD2H0e)<br />20 mm × 40 mm [http://bit.ly/2qcXwTz](http://bit.ly/2qcXwTz) |
| Solid V Wheel™ kit | $35 / 20 pcs | Wheels that ride in the V-groove | [http://bit.ly/2pIwxfD](http://bit.ly/2pIwxfD) |
| Smooth idler pulley wheel kit | $31.90 / 20 pcs | Idler wheels for belt routing without fixing the belt directly to the rail | [http://bit.ly/2oO80or](http://bit.ly/2oO80or) |
| Eccentric spacer 6 mm | $19.90 / 20 pcs | Off-center spacer to tension one side of the wheel set | [http://bit.ly/2n1N4y7](http://bit.ly/2n1N4y7) |
| GT2 timing belt | ~1200 JPY / 5 m | GT2, 2 mm pitch | [http://amzn.to/2nLpgf1](http://amzn.to/2nLpgf1) |
| Pulley | ~680 JPY (40-tooth example) | Match 2 mm pitch to the belt; tooth count affects speed vs. torque | [http://amzn.to/2nu9enP](http://amzn.to/2nu9enP) |
| T-nut M5 | $3.50 / 25 pcs | For fixing belts and plates to extrusion | [http://bit.ly/2n1Hufa](http://bit.ly/2n1Hufa) |
| Low-profile screw M5 × 6 mm | $0.10 each | For fixing belt to rail | [http://bit.ly/2nOt6W6](http://bit.ly/2nOt6W6) |
| Low-profile screw M5 × 8 mm | $10.99 / 100 pcs | For plates and corner brackets | [http://bit.ly/2ntViu7](http://bit.ly/2ntViu7) |
| Idler pulley plate | $3.75 each | Mounts idler pulleys to the rail | [http://bit.ly/2oqnR0d](http://bit.ly/2oqnR0d) |
| Motor mount plate | $4.50 each | Mounts stepper to rail | [http://bit.ly/2n1LWdT](http://bit.ly/2n1LWdT) |
| Gantry plate Universal | $7.95 each | Larger plate for moving assemblies; use the 20 mm plate below if you do not need the size | [http://bit.ly/2nOqKXA](http://bit.ly/2nOqKXA) |
| Gantry plate 20 mm | $4.50 each | Smaller moving plate; very useful | [http://bit.ly/2oqmZZA](http://bit.ly/2oqmZZA) |
| 90° inside bracket | $5 / 10 pcs | Inside-corner bracket; angular accuracy is so-so—cast corners below may be better | [http://bit.ly/2oy2LtY](http://bit.ly/2oy2LtY) |
| 90° cast corner | $0.75 each | Corner piece screwed to extrusion; needs T-nuts and low-profile screws to avoid interference | [http://bit.ly/2oqqgs0](http://bit.ly/2oqqgs0) |

* Build ideas and guides are collected on the OpenBuilds site:<br />
[http://openbuilds.org/](http://openbuilds.org/)

## Related add-ons

* **ofxGrbl** — this add-on (Grbl from openFrameworks).

* **ofxUI** — used for the UI.

* **ofxXmlSettings** — used to save ofxUI settings.

## Usage (work in progress)

* **GrblManager** — ofxGrbl example (YouTube)<br />
[![GrblManager : ofxGrbl example ](http://img.youtube.com/vi/54ps6AzPNp4/0.jpg)](http://www.youtube.com/watch?v=54ps6AzPNp4 "GrblManager : ofxGrbl example ")

## Configuring Grbl

Change settings by sending serial commands. The Arduino Serial Monitor works; send commands with a **carriage return (CR)** line ending.

See the official guide:<br />
[https://github.com/grbl/grbl/wiki/Configuring-Grbl-v0.9](https://github.com/grbl/grbl/wiki/Configuring-Grbl-v0.9)

### Show current settings

```
$$
```

You should get something like:

```
$0=3 (step pulse, usec)
$1=1 (step idle delay, msec)
$2=0 (step port invert mask:00000000)
$3=0 (dir port invert mask:00000000)
$4=0 (step enable invert, bool)
$5=0 (limit pins invert, bool)
$6=0 (probe pin invert, bool)
$10=3 (status report mask:00000011)
$11=10.000 (junction deviation, mm)
$12=0.002 (arc tolerance, mm)
$13=0 (report inches, bool)
$20=0 (soft limits, bool)
$21=0 (hard limits, bool)
$22=0 (homing cycle, bool)
$23=0 (homing dir invert mask:00000000)
$24=25.000 (homing feed, mm/min)
$25=500.000 (homing seek, mm/min)
$26=250 (homing debounce, msec)
$27=1.000 (homing pull-off, mm)
$100=20.000 (x, step/mm)
$101=20.000 (y, step/mm)
$102=20.000 (z, step/mm)
$110=20000.000 (x max rate, mm/min)
$111=20000.000 (y max rate, mm/min)
$112=20000.000 (z max rate, mm/min)
$120=500.000 (x accel, mm/sec^2)
$121=500.000 (y accel, mm/sec^2)
$122=500.000 (z accel, mm/sec^2)
$130=1301.470 (x max travel, mm)
$131=833.460 (y max travel, mm)
$132=994.120 (z max travel, mm)
ok
```

### Change a setting

Send the parameter number and value, for example to set the X-axis maximum feed rate to 15000 mm/min:

```
$110=15000
```

### Computing steps per millimeter

To scale travel correctly you need steps per mm from:

* Steps per motor revolution (`steps`)
* Driver microstep setting (`micros`)
* Belt pitch (mm)
* Pulley tooth count (`teeth`)

Formula:

```
(steps * micros) / (belt_pitch_mm * teeth) = steps/mm
```

Example:

| Parameter | Value |
| ------------- | ------------- |
| Steps per revolution | 200 |
| Microsteps | 4 |
| Belt pitch | 2 mm |
| Pulley teeth | 20 |

```
(200 * 4) / (2 * 20) = 20 steps/mm
```

Apply per axis via serial commands, for example:

```
$100=20
$101=20
$102=20
```

## Loading paths from Illustrator

Pipeline:

**Illustrator (.ai) → DXF (.dxf) → G-code (.gcode / .ngc / .nc)**

### Software

**dxf2gcode** — free tool that converts DXF to G-code<br />
[https://sourceforge.net/projects/dxf2gcode/](https://sourceforge.net/projects/dxf2gcode/)

### Steps

1. Export paths from Illustrator to DXF. Reference (Japanese page): [http://mathrax.sakura.ne.jp/kuze_jp/p-comp/cad/page003.html](http://mathrax.sakura.ne.jp/kuze_jp/p-comp/cad/page003.html)
2. Open the DXF in dxf2gcode and export `.gcode` / `.ngc`.
3. Load the output file with the LOAD icon or by drag-and-drop into the app.
