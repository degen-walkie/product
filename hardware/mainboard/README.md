# Mainboard (rev 7)

| File | What |
| --- | --- |
| [`schematic/`](schematic/) | Schematic pages as PNG |
| [`netlist.json`](netlist.json) | Every part, every pin and the net it connects to, plus a net-to-pins index |
| [`BOM.csv`](BOM.csv) | Bill of materials with manufacturer and LCSC part numbers |

## Schematic

See [`schematic/`](schematic/). Each page is exported from the EDA project as a PNG.

## Power

| Rail | Source | Used by |
| --- | --- | --- |
| VBUS | USB-C | AXP2101 input, back-clip 5 V (switched) |
| BAT+ | Li-ion cell | AXP2101 |
| VSYS | AXP2101 system output | Speaker amp, backlight anode, motor |
| 3V3 | AXP2101 DCDC1 (on at power-up) | ESP32-S3, TCA9555, codec digital, pull-ups |
| AVDD | AXP2101 ALDO1 | Codec analog, microphone |
| LCD_3V3 | AXP2101 ALDO2 | Display and touch |
| CAM_2V8 | AXP2101 ALDO3 (2.8 V) | Camera analog / IO |
| CAM_DVDD | AXP2101 ALDO4 | Camera core |

ALDO1 to ALDO4 are off at power-up and must be enabled by firmware. Keep DCDC1 at 3.3 V. The TS pin is tied to a fixed 10 k resistor, so TS temperature sensing must be disabled in the AXP2101 registers or the battery will not charge.

## I2C bus (IO8 = SDA, IO9 = SCL, 2.2 k pull-ups)

| Device | Address |
| --- | --- |
| AXP2101 PMIC | 0x34 |
| ES8388 codec (CE low) | 0x10 |
| TCA9555 IO expander (A0..A2 low) | 0x20 |
| GT911 touch | 0x5D or 0x14 (set at reset) |
| OV2640 camera (SCCB) | 0x30 |

## ESP32-S3 pin map

| GPIO | Net | Function |
| --- | --- | --- |
| IO0 | LCD_BL_PWM | Backlight PWM (also the BOOT strap, 4.7 k pull-up) |
| IO1 | EXP_INT | TCA9555 interrupt (open drain) |
| IO2 | I2S_DIN | Codec ADC data |
| IO3 | WHEEL_A | Wheel encoder phase A |
| IO4 | I2S_DOUT | Codec DAC data |
| IO5 | I2S_MCLK | Codec master clock |
| IO7 | I2S_LRCK | Codec word clock |
| IO8 | I2C_SDA | I2C data |
| IO9 | I2C_SCL | I2C clock |
| IO10 | LCD_CS | Display chip select |
| IO11 | LCD_MOSI | Display data |
| IO12 | LCD_SCK | Display clock |
| IO13 | LCD_DC | Display data / command |
| IO14 | LCD_TE | Display tearing effect |
| IO15, IO16, IO17, IO18, IO21, IO38, IO39, IO40 | CAM_D0 .. CAM_D7 | Camera data |
| IO19 / IO20 | USB_DN / USB_DP | Native USB |
| IO33 | I2S_BCLK | Codec bit clock |
| IO41 | CAM_PCLK | Camera pixel clock |
| IO42 | CAM_VSYNC | Camera vsync |
| IO47 | CAM_HREF | Camera href |
| IO48 | CAM_XCLK | Camera clock |
| TXD0 / RXD0 | CLIP_TXD / CLIP_RXD | Back-clip UART |

On the N4R2 module only IO26 is reserved for the PSRAM; IO34..IO37 are unused. IO45 and IO46 are unconnected straps (internal pull-downs). EN has a 10 k / 1 uF delay.

## TCA9555 pin map

| Pin | Net | Function |
| --- | --- | --- |
| P00 | LCD_RST | Display reset |
| P01 | TP_RST | Touch reset |
| P02 | TP_INT | Touch interrupt |
| P03 | PA_EN | Speaker amplifier enable |
| P05 | LED_R_K | Status light red (active low) |
| P06 | KEY_TRADE | Trade key (active low) |
| P10 | WHEEL_B | Wheel encoder phase B |
| P11 | WHEEL_SW | Wheel press |
| P12 | AXP_IRQ | PMIC interrupt |
| P13 | LED_G_K | Status light green (active low) |
| P14 | CLIP_DET | Back-clip detect |
| P15 | CLIP_RI | Back-clip ring indicator |
| P16 | CLIP_CHG_EN | Back-clip 5 V enable |
| P17 | VIBE | Vibration motor |

The blue channel of the status light is driven by the AXP2101 CHGLED pin.

## Connectors

| Ref | Type | Function |
| --- | --- | --- |
| J_LCD | 40-pin 0.5 mm FPC | Display (ST7796U, 4-wire SPI, IM0..IM2 high). Pad number = 41 - cable pin |
| J_TP | 8-pin 0.5 mm FPC | GT911 touch. Pad number = 9 - cable pin |
| J_CAM | 24-pin 0.5 mm FPC, bottom contact | OV2640 module (ESP32-S3-EYE pinout). Pad number = 25 - cable pin |
| J_USB | Mid-mount USB-C | Charging, native USB, firmware updates |
| J_WHEEL | 6 pads | Wheel daughterboard: 1 WHEEL_A, 2 WHEEL_B, 3 WHEEL_SW, 4 KEY_TRADE, 5 GND, 6 GND |
| J_CLIP1..6 | Spring pogo pins | Back-clip: 1 CLIP_5V, 2 GND, 3 TXD, 4 RXD, 5 RI, 6 DET |
| J_BAT, J_SPK, J_MOT | Solder pads | Battery, speaker, coin vibration motor |

The connectors sit on the back of the board, so FPC pad numbers are mirrored relative to the cable pin numbers, as noted above.

## Backlight

The 8 backlight LEDs run from VSYS through a low-dropout constant-current sink: LMV321 op-amp, AO3400 MOSFET and a 1 ohm sense resistor. The current setpoint comes from the IO0 PWM through a 27 k / 1 k divider and a 1 uF filter: 100 % duty gives about 118 mA. Use a PWM of 20 kHz or more, and start the PWM only after boot, since IO0 is the boot strap.

## Known issues on rev 7

- **VMID not tied to VSYS (board does not power up).** AXP2101 pin 36 (VMID) only goes to C6. The datasheet (Fig. 7-1) ties VMID to VSYS: VBUS reaches the system only through the internal RBFET to VMID. On rev 7, USB gives 5 V on VBUS and 0 V on VSYS. Fix on rev 7: one wire from the VMID end of C6 to the VSYS end of C8 (C5, on VBUS, sits next to C6: don't pick it by mistake). Rev 8 merges the VMID net into VSYS.
- **Camera FPC clearance.** Five parts in front of the J_CAM opening block the camera cable and are not populated (DNP) on rev 7 boards: R35 (CAM_RESET pull-up), R36 (CAM_PWDN pull-down), C52 and C53 (camera supply decoupling) and R21 (status light blue). CAM_RESET and CAM_PWDN float as a result; check the OV2640 responds on SCCB at bring-up, and wire RESET to CAM_2V8 and PWDN to GND if it does not. The blue status light channel is unused on rev 7. These parts move behind the connector on the next revision.
- **Pogo pins.** The BOM uses YTC1P-3515-01 (LCSC C5221292), which protrude 0.4 mm from the back cover.
