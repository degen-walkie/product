# Wheel daughterboard (rev 7)

A small 2-layer board (0.8 mm) that carries the thumb wheel encoder and the trade key on the right edge of the device. It connects to the mainboard with five short wires. All parts are hand soldered.

## Parts

| Ref | Part | LCSC | Qty |
| --- | --- | --- | --- |
| U1 | Thumb wheel encoder SIQ-02FVS3 (includes the 14.5 mm knurled wheel) | C2925423 | 1 |
| SW1 | Side push switch K2-1806SQ-A3DW-04 (trade key) | C128945 | 1 |

The encoder's own knurled wheel is the thumb wheel. No separate wheel cap is needed.

## Wiring to the mainboard

| Daughterboard pad (silkscreen) | Mainboard J_WHEEL pad | Net | Goes to |
| --- | --- | --- | --- |
| A | 1 | WHEEL_A | ESP32-S3 IO3 |
| B | 2 | WHEEL_B | TCA9555 P10 |
| S | 3 | WHEEL_SW | TCA9555 P11 (wheel press) |
| T | 4 | KEY_TRADE | TCA9555 P06 (trade key) |
| G | 5 or 6 | GND | Ground |

The encoder common (C) and the second trade key contact go to GND on the daughterboard. WHEEL_A, WHEEL_B, WHEEL_SW and KEY_TRADE have 10 k pull-ups to 3V3 on the mainboard, and WHEEL_A / WHEEL_B have 10 nF debounce capacitors.

## Checking a bare encoder

Rotating the wheel alternately connects A to C and B to C (two-phase quadrature, 15 detents). Pressing the wheel connects S to C.
