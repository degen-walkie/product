// Shared price engine for the three mock venues. Prices start from Hyperliquid's
// public mids when the browser can reach them, otherwise from fixed values, then
// move as a random walk. Each venue quotes with its own small offset.

export const COINS = ['BTC', 'ETH', 'SOL', 'HYPE'];
const START = { BTC: 118240, ETH: 4412.5, SOL: 212.4, HYPE: 24.18 };
const DAY_CHANGE = { BTC: -0.008, ETH: 0.012, SOL: 0.031, HYPE: 0.064 };
const VOL = { BTC: 0.00025, ETH: 0.00035, SOL: 0.0005, HYPE: 0.0008 }; // per-second sigma (lively, for a demo)
const FUNDING = { BTC: 0.0000125, ETH: 0.0000098, SOL: 0.0000131, HYPE: 0.0000215 };
export const VENUE_OFFSET = { hl: 0, pac: 0.0002, lit: -0.00015 };

const state = { mid: {}, prev: {}, anchor: {}, live: {} };

function gauss() {
  let u = 0, v = 0;
  while (!u) u = Math.random();
  while (!v) v = Math.random();
  return Math.sqrt(-2 * Math.log(u)) * Math.cos(2 * Math.PI * v);
}

export async function initMarket() {
  let seeded = false;
  try {
    const ctl = new AbortController();
    const t = setTimeout(() => ctl.abort(), 2500);
    const r = await fetch('https://api.hyperliquid.xyz/info', {
      method: 'POST', headers: { 'Content-Type': 'application/json' }, body: '{"type":"allMids"}', signal: ctl.signal,
    });
    clearTimeout(t);
    const mids = await r.json();
    for (const c of COINS) if (mids[c]) { START[c] = +mids[c]; seeded = true; }
  } catch { /* offline or blocked: use the built-in prices */ }
  for (const c of COINS) {
    state.mid[c] = START[c];
    state.anchor[c] = START[c];
    state.prev[c] = START[c] / (1 + DAY_CHANGE[c]);
  }
  return seeded;
}

export function tick(dt) {
  for (const c of COINS) {
    const m = state.mid[c];
    const revert = (state.anchor[c] - m) / state.anchor[c] * 0.002 * dt;
    state.mid[c] = m * Math.exp(VOL[c] * Math.sqrt(dt) * gauss() + revert);
  }
}

export const mid = (coin, venue = 'hl') => state.mid[coin] * (1 + (VENUE_OFFSET[venue] || 0));
export const prevDay = (coin, venue = 'hl') => state.prev[coin] * (1 + (VENUE_OFFSET[venue] || 0));
export const funding = (coin) => FUNDING[coin];
export const openInterest = (coin) => ({ BTC: 31840, ETH: 412300, SOL: 2840000, HYPE: 9120000 })[coin];

/* ---- candles: deterministic history ending at the live price ---- */
const cache = {};
function seeded(seed) {
  let s = seed >>> 0 || 1;
  return () => { s = (s * 16807) % 2147483647; return s / 2147483647; };
}
function seedOf(str) { let h = 2166136261; for (const ch of str) h = Math.imul(h ^ ch.charCodeAt(0), 16777619); return h >>> 0; }

export function candles(coin, ivMs, startMs, endMs, venue = 'hl') {
  const now = Date.now();
  const lastT = Math.floor(now / ivMs) * ivMs;
  const key = `${coin}:${ivMs}`;
  let c = cache[key];
  if (!c || c.lastT !== lastT) {
    const n = 200;
    const rnd = seeded(seedOf(key + ':' + Math.floor(lastT / ivMs / 50)));
    const sig = VOL[coin] * Math.sqrt(ivMs / 1000) * 0.8;
    const closes = new Array(n);
    closes[n - 1] = state.mid[coin];
    for (let i = n - 2; i >= 0; i--) {
      const z = (rnd() + rnd() + rnd() - 1.5) * 2;
      closes[i] = closes[i + 1] * Math.exp(-sig * z - DAY_CHANGE[coin] * ivMs / 86400000 * 0.6);
    }
    const bars = [];
    for (let i = 0; i < n; i++) {
      const o = i ? closes[i - 1] : closes[0] * (1 - sig * 0.3);
      const cl = closes[i];
      const span = Math.abs(cl - o) + cl * sig * 0.6 * rnd();
      bars.push({ t: lastT - (n - 1 - i) * ivMs, o, h: Math.max(o, cl) + span * rnd() * 0.6, l: Math.min(o, cl) - span * rnd() * 0.6, c: cl });
    }
    c = cache[key] = { lastT, bars };
  }
  const last = c.bars[c.bars.length - 1];
  const live = state.mid[coin];
  last.c = live;
  last.h = Math.max(last.h, live);
  last.l = Math.min(last.l, live);
  const k = 1 + (VENUE_OFFSET[venue] || 0);
  return c.bars
    .filter((b) => b.t >= startMs - ivMs && b.t <= endMs)
    .map((b) => ({ t: b.t, T: b.t + ivMs - 1, o: b.o * k, h: b.h * k, l: b.l * k, c: b.c * k }));
}
