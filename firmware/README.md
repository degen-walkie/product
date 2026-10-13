# Firmware

The Degen Walkie firmware: C with [LVGL](https://lvgl.io) on the ESP32-S3. The same source builds two ways:

| Target | How | Output |
| --- | --- | --- |
| ESP32-S3 (the device) | ESP-IDF 5.3, `idf.py build` | `build/degen_walkie.bin` |
| Browser simulator | Emscripten, `tools/build_sim.py` | `../simulator/public/fw/walkie.wasm` |

**Try it:** [degenwalkie.trade/sim](https://degenwalkie.trade/sim) runs this firmware in your browser: scripted demos against mock venues, or **Real** mode on the Hyperliquid and Pacifica testnets ([guide](../docs/real-trading.md)).

> **Status.** The firmware runs end to end in the simulator. It has not run on a device yet: rev 7 mainboards are in bring-up. Expect the board layer (`main/`) to need fixes on first power-up, for example the display orientation, colour order, the GT911 address and the AXP2101 charger settings.

## Layout

```
core/                 Platform-independent firmware (shared by both targets)
  include/dw_hal.h    The hardware abstraction: the only way the core reaches the board
  dw_app.c            Start-up, input routing, the voice path
  dw_model.[ch]       Markets, positions, accounts, rooms, news
  venues/             Venue clients: hl.c, pacifica.c, lighter.c, relay.c
  crypto/             Keccak-256, secp256k1 and Ed25519 signing, MessagePack, base58
  ui/                 UI kit and screens (rooms, chart, ticket, confirm sheet, accounts…)
  fonts/              Archivo, Inter, JetBrains Mono + a CJK fallback (Noto Sans SC/KR)
  lv_conf.h           LVGL configuration (both targets)
main/                 ESP32-S3 board layer: AXP2101, TCA9555, ST7796U, GT911, wheel,
                      keys, status light, Wi-Fi, HTTP/WebSocket, NVS
sim/                  WebAssembly board layer: canvas, pointer, the page's mock network
components/dw_core/   ESP-IDF component wrapper around core/ and third_party/
third_party/          secp256k1 (Bitcoin Core), Monocypher, cJSON
tools/                build_sim.py, gen_fonts.py
```

## What it does

- **Power and setup.** Off → hold power → boot → first-time setup (language, Wi-Fi, PIN, link a venue). Short press: screen off with the room still playing. Hold: slide-to-power-off.
- **Rooms.** A world room and one room per asset. Hold to talk: the ES8388 records, the relay turns it into text, and every listener gets it in their own language. No linked account needed to talk; an unlinked device shows as a guest.
- **Trading.** Ticket with long/short, market/limit, leverage, size by share of free margin, take profit and stop loss. One confirmation sheet; only the trade key signs. Close / half / add from the chart, cancel resting orders from the room.
- **Voice orders and Ask.** Speak an order and it becomes a ticket; ask about your book and a read-only AI answers.
- **Navigation.** Thumb wheel and trade key for everything, or touch: tap, scroll, swipe between top pages, pull down for quick settings.
- **Networks.** Demo (mocks), testnet or mainnet, chosen at runtime (`core/dw_config.c`). Lighter is locked outside the demo until its signer is ported.

## How trading works

The device makes its own trading key for each venue. You approve that key from your main wallet once, on your phone. From then on the device signs orders locally; nothing else holds the key. The key is limited by the venue:

| Venue | Device key | Signing | What it cannot do |
| --- | --- | --- | --- |
| Hyperliquid | Agent (API) wallet, secp256k1 | EIP-712 "phantom agent" over the msgpack hash of the action | Withdraw or transfer. Those need the main wallet's signature. |
| Pacifica | Agent wallet, Ed25519 | Ed25519 over sorted, compact JSON; base58 | The firmware never builds a withdrawal. |
| Lighter | API key | See note below | Withdraw anywhere except the account's own address. |

Every order goes through one confirmation sheet, and only the physical trade key signs it. Touch can build a ticket but cannot sign.

**Lighter note.** Lighter signs transactions with Schnorr over the ECgFp5 curve with Poseidon2 hashing. That signer is not ported to C yet, so `lighter.c` signs `tx_info` with an Ed25519 stand-in, which the simulator's mock accepts. Endpoints, transaction types, integer scaling and nonces follow the real API.

## Relay protocol

Rooms, news, the AI and the linking handshake go through the Degen Walkie relay (a hosted service, not in this repository). It never sees keys: the device only sends what is below.

| Call | Device sends | Gets back |
| --- | --- | --- |
| `GET /v1/rooms`, `GET /v1/rooms/:id/history?lang=XX` | room id, display language | room list, recent messages |
| `GET /v1/news` | nothing | headlines |
| `POST /v1/ask` | the question, display language, position summary (venue, coin, size, entry, liquidation, leverage, PnL, mark, funding) | an answer |
| `GET /v1/link/:session` | the random session id from the QR | the approved account address |
| `POST /v1/stt?lang=xx` | the clip recorded while talk is held (8-bit mu-law WAV, 16 kHz) | the text |
| `WS /v1/ws` | `hello {addr, dev, lang}` (the linked account address, if any, and a random device id), `join {room}`, `say {room, text, lang, addr}` | `msg`, `news`, `listening` |

## Build for the device

ESP-IDF 5.3 or newer:

```sh
cd firmware
idf.py set-target esp32s3
idf.py menuconfig          # Degen Walkie → Wi-Fi SSID and password (development builds)
idf.py build flash monitor
```

Or with Docker, no local toolchain:

```sh
docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.1 \
  sh -c "idf.py set-target esp32s3 && idf.py build"
```

LVGL and the display, touch and WebSocket drivers come from the ESP component registry (`main/idf_component.yml`).

**Production builds** add secure boot v2, flash encryption and encrypted NVS, so the keys are encrypted at rest and only signed firmware runs:

```sh
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.secure" build
```

This burns eFuses and cannot be undone. Don't use it on a development board.

## Build the simulator

Needs [emsdk](https://emscripten.org) on the PATH and an LVGL v9.2 checkout:

```sh
git clone --depth 1 -b v9.2.2 https://github.com/lvgl/lvgl.git ../lvgl
LVGL_DIR=../lvgl python tools/build_sim.py
cd ../simulator && npm install && npm run build
npx serve public            # or any static server
```

## Not done yet

- Lighter's real transaction signer (see above); Lighter is locked outside the demo until then.
- Wi-Fi setup by scanning a QR with the rear camera. Development builds take Wi-Fi from menuconfig.
- OTA updates, the camera. With the CJK font the image is 2.4 MB; OTA needs two app slots, so it needs the 8 MB flash module or a smaller font.
- The blue channel of the status light is not fitted on rev 7, so blue shows as green + red.

## Licences

Firmware: MIT (see [`LICENSE`](LICENSE)). Third-party code keeps its own licence: LVGL (MIT), secp256k1 (MIT), Monocypher (CC0 / BSD-2-Clause), cJSON (MIT). Fonts: Archivo, Inter, JetBrains Mono, Noto Sans SC and Noto Sans KR, SIL Open Font License 1.1.
