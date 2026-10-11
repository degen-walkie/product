# Hardware

Degen Walkie Genesis Edition hardware, revision 7.

| Folder | Contents |
| --- | --- |
| [`mainboard/`](mainboard/) | Schematic, netlist, bill of materials, pin map and bring-up notes for the main PCB |
| [`wheelboard/`](wheelboard/) | Thumb wheel and trade key daughterboard: parts and wiring |
| [`renders/`](renders/) | Illustrative render of the mainboard |

![Mainboard render](renders/mainboard-render.svg)

*Illustrative render of the mainboard, component side. Outline and part placement follow the real board; the copper traces are decorative.*

## What is and is not published

Published: schematic, netlist, bill of materials, pin assignments, connector pinouts, known issues.

Not published: the PCB layout (copper, placement and Gerber files) and the EDA project. The netlist and schematic are enough to read, audit and write firmware for the board.

## The device at a glance

- **MCU:** ESP32-S3-MINI-1-N4R2 (Wi-Fi 2.4 GHz, Bluetooth LE 5, 4 MB flash, 2 MB PSRAM)
- **Display:** 4.0-inch 320 x 480 IPS (ST7796U, 4-wire SPI) with GT911 capacitive touch
- **Power:** AXP2101 PMIC, single-cell Li-ion (about 1000 mAh), USB-C charging
- **Audio:** ES8388 codec, MEMS microphone, NS4150B speaker amplifier
- **Controls:** thumb wheel encoder (SIQ-02FVS3) and trade key on a daughterboard, power key, reset and boot keys
- **IO expander:** TCA9555 for keys, resets and enables
- **Status light:** side-emitting RGB LED on the top edge
- **Camera:** OV2640 module on a 24-pin FPC, for QR codes (Wi-Fi setup, account linking)
- **Back-clip contacts:** six spring pogo pins for the planned 4G back-clip
- **Board:** 4 layers, 0.8 mm, ENIG, all parts on one side

## License

The hardware design files in this folder are licensed under the CERN Open Hardware Licence Version 2 - Strongly Reciprocal ([`LICENSE`](LICENSE)).
