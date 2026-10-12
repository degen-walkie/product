# Simulator

A browser simulator for the Degen Walkie. Live at **[degenwalkie.trade/sim](https://degenwalkie.trade/sim)**.

The screen runs the real firmware ([`../firmware`](../firmware)), compiled to WebAssembly. Two modes:

- **Demo**: pick a demo (power on, first-time setup, open / close a position, limit order, TP/SL, live room, Ask AI, venues, voice order, quick panel) and press Play. The page drives the device and highlights each key press, tap and swipe. Runs against in-page mocks.
- **Real**: the device starts powered off, you set it up, link your own wallets and trade on the Hyperliquid and Pacifica **testnets** (or mainnet, behind a warning). See **[the guide](../docs/real-trading.md)**.

This folder is everything around the device:

| File | What |
| --- | --- |
| `public/index.html`, `style.css` | The page: the device, its controls, the phone panel, the network log |
| `public/fw/` | The firmware build (`walkie.js`, `walkie.wasm`) from `firmware/tools/build_sim.py` |
| `src/host.js` | The board as the firmware sees it: display, keys, status light, battery, storage, mic |
| `src/net.js` | Routes the firmware's HTTP and WebSocket calls to the mocks, and logs them |
| `src/mock_hyperliquid.js` | Hyperliquid `/info`, `/exchange` and WebSocket |
| `src/mock_pacifica.js` | Pacifica REST and WebSocket |
| `src/mock_lighter.js` | Lighter REST (`sendTx`, accounts, order books, candles) |
| `src/mock_relay.js` | The Degen Walkie relay: rooms, news, the AI, account linking |
| `src/market.js`, `src/ledger.js` | Prices, candles and the simulated accounts |
| `src/phone.js` | Demo main wallets: approving the device's keys, and a withdrawal attack |
| `src/demo.js` | The scripted demos and their player |
| `src/wallet.js` | Real mode: MetaMask/Rabby (Hyperliquid) and Phantom/Backpack (Pacifica) approvals |
| `public/link.html`, `src/link.js` | The page a phone opens after scanning the device's QR |

## What is real and what is mocked

- **Real:** the firmware, its UI, the order flow and the signing. Hyperliquid requests are signed with EIP-712 over a msgpack hash, Pacifica requests with Ed25519 over sorted JSON. The mocks verify those signatures the way the venues do, and the Network tab shows each check.
- **Mocked:** the three venues, the relay and the market. Endpoints and payloads follow the real APIs. Prices start from Hyperliquid's public mids when the browser can reach them, then follow a random walk. Nothing touches real funds.
- **Stand-ins:** Lighter's signature (see the firmware README), the AI (preset answers built from your live positions) and voice (typed text, or Chrome's speech recognition).

Each demo starts from a clean simulated device. In Real mode, each network keeps its own device state in `localStorage` (Settings → Factory reset clears it).

## Run locally

```sh
npm install
npm run build        # bundles src/ into public/sim.js
npx serve public     # any static server works
```

Rebuild the firmware first if you changed it: `python ../firmware/tools/build_sim.py`.

## Controls

| Key | Device |
| --- | --- |
| ↑ / ↓, or scroll on the screen | Roll the thumb wheel |
| Esc | Press the wheel (back, or the menu at the top level) |
| Enter / Space | Trade key: open, then sign on the confirmation sheet |
| Hold M | Hold to talk |
| P | Power key, short press (screen off, rooms keep playing) |
| Shift + P | Power key held: power on, or the power-off sheet |

The same controls are buttons under the device.

Touch works on the screen. Signing does not: only the trade key signs.

MIT licence.
