// The board, as far as the firmware can tell: display, keys, status light, battery,
// storage, network and mic. Implements Module.dwHost for sim/hal_web.c.
import { http, socket } from './net.js';

const LED = ['off', 'green', 'blue', 'red', 'yellow', 'purple'];
const MODE = ['solid', 'blink', 'slow'];
export const IN = { UP: 0, DOWN: 1, PRESS: 2, TRADE: 3, POWER: 4, PTT_DOWN: 5, PTT_UP: 6 };

export function createHost(ui) {
  let M = null;
  const sockets = new Map();
  let battery = 86, charging = false;
  let frameQueued = false;

  const toC = (s) => (s === null || s === undefined ? 0 : M.stringToNewUTF8(s));

  function blit() {
    frameQueued = false;
    const fb = M._dw_sim_fb() >> 1;
    const src = M.HEAPU16.subarray(fb, fb + 320 * 480);
    const dst = ui.pixels;
    for (let i = 0; i < src.length; i++) {
      const p = src[i];
      const r = (p >> 11) & 31, g = (p >> 5) & 63, b = p & 31;
      dst[i] = 0xff000000 | (((b << 3) | (b >> 2)) << 16) | (((g << 2) | (g >> 4)) << 8) | ((r << 3) | (r >> 2));
    }
    ui.paint();
  }

  const host = {
    attach(mod) { M = mod; },
    present() { if (!frameQueued) { frameQueued = true; requestAnimationFrame(blit); } },
    led(c, m) { ui.led(LED[c] || 'off', MODE[m] || 'solid'); },
    vibrate(ms) { ui.vibrate(ms); },
    backlight(on) { ui.backlight(!!on); },
    power() { return Math.round(battery) | (charging ? 1 << 8 : 0) | (charging ? 1 << 9 : 0); },
    http(id, method, url, headers, body) {
      http(method, url, headers, body).then(({ status, body: text }) => {
        const p = text ? M.stringToNewUTF8(text) : 0;
        M._dw_sim_http_done(id, status, p, text ? M.lengthBytesUTF8(text) : 0);
      });
    },
    wsOpen(id, url) {
      const s = socket(url, (text) => M._dw_sim_ws_msg(id, M.stringToNewUTF8(text), M.lengthBytesUTF8(text)), (open) => M._dw_sim_ws_state(id, open));
      sockets.set(id, s);
    },
    wsSend(id, text) { sockets.get(id)?.send(text); },
    wsClose(id) { sockets.get(id)?.close(); sockets.delete(id); },
    storeGet(k) { return localStorage.getItem('dw.fw.' + k); },
    storeSet(k, v) { if (v === null) localStorage.removeItem('dw.fw.' + k); else localStorage.setItem('dw.fw.' + k, v); },
    voice(on) {
      if (on) ui.listen(true);
      else ui.listen(false).then((text) => M._dw_sim_voice(toC(text || '')));
    },
    qr(payload) { ui.qr(payload); },
  };

  host.input = (code) => M && M._dw_sim_input(code);
  host.touch = (x, y, down) => M && M._dw_sim_touch(x, y, down ? 1 : 0);
  host.setCharging = (v) => { charging = v; };
  host.battery = () => ({ battery, charging });
  setInterval(() => {
    battery = charging ? Math.min(100, battery + 0.5) : Math.max(5, battery - 0.02);
  }, 3000);
  return host;
}
