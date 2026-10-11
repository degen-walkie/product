# Firmware

Not published yet. The Degen Walkie firmware will be released here during the pre-order, before Genesis ships.

What it will cover:

- Secure boot, flash encryption and encrypted storage of the trading keys you link to the device
- Order signing for Hyperliquid, Pacifica and Lighter, with the physical trade key as the only way to send an order
- Display, touch, thumb wheel, trade key and status light
- Audio for the live rooms and the AI (ES8388 codec)
- The MCP interface that lets AI agents read and propose, but never send

Target: ESP32-S3 (ESP32-S3-MINI-1-N4R2). Pin assignments are in [`../hardware/mainboard/README.md`](../hardware/mainboard/README.md).
