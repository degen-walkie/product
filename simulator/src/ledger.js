// Accounts held by the mock venues: collateral, positions, resting orders.
// One ledger per venue, persisted in localStorage so a reload keeps your book.
import * as mk from './market.js';

const KEY = 'dw.mock';
const MMR = 0.5; // maintenance margin as a fraction of initial margin

export const db = load();

function fresh() {
  return {
    hl: { accounts: {}, agents: {}, nonces: {} },        // agents: agentAddr(lower) -> { user, name }
    pac: { accounts: {}, agents: {}, nextOid: 100000 },   // agents: agentPub58 -> account
    lit: { accounts: {}, keys: {}, nonces: {} },          // keys: "acct:idx" -> pubkeyHex
    relay: { links: {} },
    oid: 1000000,
  };
}

function load() {
  try {
    const v = JSON.parse(localStorage.getItem(KEY));
    if (v && v.hl && v.pac && v.lit) return v;
  } catch { /* fresh */ }
  return fresh();
}

export function save() {
  try { localStorage.setItem(KEY, JSON.stringify(db)); } catch { /* storage full or blocked */ }
}
setInterval(save, 3000);

export function reset() {
  const f = fresh();
  for (const k of Object.keys(db)) delete db[k];
  Object.assign(db, f);
  save();
}

export function account(venue, id, seed = 0) {
  const book = db[venue].accounts;
  if (!book[id]) book[id] = { collateral: seed, positions: {}, orders: [], lev: {}, fills: [] };
  return book[id];
}

export const mark = (venue, coin) => mk.mid(coin, venue);

export function positionView(venue, acct, coin) {
  const p = acct.positions[coin];
  if (!p || !p.szi) return null;
  const m = mark(venue, coin);
  const value = Math.abs(p.szi) * m;
  const lev = p.lev || 5;
  const margin = value / lev;
  const upnl = (m - p.entry) * p.szi;
  const liq = p.szi > 0 ? p.entry * (1 - 1 / lev + MMR / lev) : p.entry * (1 + 1 / lev - MMR / lev);
  return { coin, szi: p.szi, entry: p.entry, value, margin, upnl, liq, lev, roe: upnl / (Math.abs(p.szi) * p.entry / lev) };
}

export function summary(venue, acct) {
  let upnl = 0, used = 0, ntl = 0;
  for (const coin of Object.keys(acct.positions)) {
    const v = positionView(venue, acct, coin);
    if (!v) continue;
    upnl += v.upnl;
    used += v.margin;
    ntl += v.value;
  }
  for (const o of acct.orders) if (!o.kind || o.kind === 'LIMIT') used += (o.sz * o.px) / (acct.lev[o.coin] || 5);
  const equity = acct.collateral + upnl;
  return { equity, used, ntl, free: Math.max(0, equity - used), upnl };
}

/* Fill against the mock book at px. Returns { ok, error }. */
export function fill(venue, acct, coin, buy, sz, px, reduceOnly = false) {
  const p = acct.positions[coin] || { szi: 0, entry: 0, lev: acct.lev[coin] || 5 };
  const signed = buy ? sz : -sz;
  if (reduceOnly) {
    if (!p.szi || Math.sign(p.szi) === Math.sign(signed)) return { ok: false, error: 'Reduce only order would increase position.' };
    if (Math.abs(signed) > Math.abs(p.szi)) return fill(venue, acct, coin, buy, Math.abs(p.szi), px, true);
  } else {
    const lev = acct.lev[coin] || p.lev || 5;
    const s = summary(venue, acct);
    const adds = !p.szi || Math.sign(p.szi) === Math.sign(signed);
    if (adds && (sz * px) / lev > s.free + 1e-9) return { ok: false, error: 'insufficient_margin' };
  }
  const fee = sz * px * (venue === 'lit' ? 0 : venue === 'pac' ? 0.0004 : 0.00035);
  acct.collateral -= fee;
  if (!p.szi || Math.sign(p.szi) === Math.sign(signed)) {
    const n = p.szi + signed;
    p.entry = (Math.abs(p.szi) * p.entry + sz * px) / Math.abs(n);
    p.szi = n;
    p.lev = acct.lev[coin] || p.lev || 5;
  } else {
    const closing = Math.min(Math.abs(signed), Math.abs(p.szi));
    acct.collateral += (px - p.entry) * closing * Math.sign(p.szi);
    const rest = p.szi + signed;
    if (Math.abs(rest) < 1e-12) { p.szi = 0; p.entry = 0; }
    else if (Math.sign(rest) !== Math.sign(p.szi)) { p.szi = rest; p.entry = px; }
    else p.szi = rest;
  }
  if (p.szi) acct.positions[coin] = p;
  else delete acct.positions[coin];
  acct.fills.unshift({ coin, buy, sz, px, t: Date.now() });
  acct.fills.length = Math.min(acct.fills.length, 30);
  save();
  return { ok: true, px, sz };
}

/* Rest a limit order; filled by matchResting() when the price crosses. */
export function rest(acct, coin, buy, sz, px, kind = 'LIMIT') {
  const oid = db.oid++;
  if (kind !== 'LIMIT') acct.orders = acct.orders.filter((o) => !(o.coin === coin && o.kind === kind));
  acct.orders.push({ oid, coin, buy, sz, px, kind, t: Date.now() });
  save();
  return oid;
}

export function matchResting() {
  for (const venue of ['hl', 'pac', 'lit']) {
    for (const acct of Object.values(db[venue].accounts)) {
      const fired = new Set();
      acct.orders = acct.orders.filter((o) => {
        const m = mark(venue, o.coin);
        const kind = o.kind || 'LIMIT';
        if (kind === 'LIMIT') {
          if ((o.buy && m <= o.px) || (!o.buy && m >= o.px)) return !fill(venue, acct, o.coin, o.buy, o.sz, o.px).ok;
          return true;
        }
        const pos = acct.positions[o.coin];
        if (!pos || !pos.szi || fired.has(o.coin)) return false;   // position gone: the other leg is cancelled too
        // A sell trigger guards a long; a buy trigger guards a short.
        const hit = kind === 'TP' ? (o.buy ? m <= o.px : m >= o.px) : (o.buy ? m >= o.px : m <= o.px);
        if (!hit) return true;
        fill(venue, acct, o.coin, o.buy, Math.min(o.sz, Math.abs(pos.szi)), m, true);
        fired.add(o.coin);
        return false;
      });
      if (fired.size) acct.orders = acct.orders.filter((o) => !(fired.has(o.coin) && (o.kind === 'TP' || o.kind === 'SL')));
    }
  }
}

/* A demo book so the screens have something to show on first run. */
export function seedDemo(venue, acct) {
  const put = (coin, szi, lev, offset) => {
    acct.lev[coin] = lev;
    acct.positions[coin] = { szi, entry: mark(venue, coin) * (1 - offset), lev };
  };
  if (venue === 'hl') {
    acct.collateral = 4210.55;
    put('SOL', 11.8, 5, 0.022);
    put('HYPE', 40, 2, 0.06);
  } else if (venue === 'pac') {
    acct.collateral = 1350;
    put('ETH', 0.42, 10, -0.008);
  } else if (venue === 'lit') {
    acct.collateral = 620;
    put('BTC', -0.012, 3, -0.009);
  }
  save();
}
