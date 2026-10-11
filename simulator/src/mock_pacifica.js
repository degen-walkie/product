// Mock of the Pacifica REST API (https://api.pacifica.fi/api/v1) and its WebSocket.
// Signed requests are verified like Pacifica does: rebuild {type, timestamp,
// expiry_window, data} as compact JSON with sorted keys and check the Ed25519
// signature (base58) against the agent wallet bound to the account.
import * as mk from './market.js';
import * as L from './ledger.js';
import { register } from './net.js';
import { b58decode, edVerify, short, sortedJson } from './util.js';

export const BASE = 'https://api.pacifica.fi/api/v1';
const MARKETS = {
  BTC: { tick_size: '1', lot_size: '0.00001', max_leverage: 50, min_order_size: '10' },
  ETH: { tick_size: '0.1', lot_size: '0.0001', max_leverage: 50, min_order_size: '10' },
  SOL: { tick_size: '0.01', lot_size: '0.01', max_leverage: 20, min_order_size: '10' },
};
const TYPES = {
  '/orders/create_market': 'create_market_order',
  '/orders/create': 'create_order',
  '/orders/cancel': 'cancel_order',
  '/account/leverage': 'update_leverage',
  '/agent/bind': 'bind_agent_wallet',
};
const SIG_FIELDS = ['account', 'agent_wallet', 'signature', 'timestamp', 'expiry_window'];
const ok = (data) => ({ status: 200, body: { success: true, data, error: null, code: null } });
const bad = (error, code = 400) => ({ status: code, body: { success: false, data: null, error, code } });
const s = (n, d) => (+n).toFixed(d);
const db = () => L.db.pac;

function verify(path, b, note) {
  const type = TYPES[path];
  const data = {};
  for (const [k, v] of Object.entries(b)) if (!SIG_FIELDS.includes(k)) data[k] = v;
  if (type === 'bind_agent_wallet') data.agent_wallet = b.agent_wallet; // here the agent wallet is the signed payload
  const msg = sortedJson({ type, timestamp: b.timestamp, expiry_window: b.expiry_window, data });
  const signerKey = type === 'bind_agent_wallet' ? b.account : b.agent_wallet;
  let pub, sig;
  try { pub = b58decode(signerKey); sig = b58decode(b.signature); } catch { return 'Invalid signature encoding'; }
  if (!edVerify(sig, msg, pub)) { note('✗ Ed25519 signature does not match the sorted-JSON message'); return 'Verification failed'; }
  if (Date.now() > b.timestamp + b.expiry_window) return 'Request expired';
  if (type === 'bind_agent_wallet') { note(`✓ main wallet ${short(b.account)} signed bind_agent_wallet`); return null; }
  if (db().agents[b.agent_wallet] !== b.account) { note(`✗ agent ${short(b.agent_wallet)} is not bound to ${short(b.account)}`); return 'Agent wallet not authorized for this account'; }
  note(`✓ Ed25519 signature by agent ${short(b.agent_wallet)} (bound to ${short(b.account)}) over ${type}`);
  return null;
}

function positions(acct) {
  return Object.keys(acct.positions).map((coin) => {
    const v = L.positionView('pac', acct, coin);
    return { symbol: coin, side: v.szi > 0 ? 'bid' : 'ask', amount: s(Math.abs(v.szi), 5), entry_price: s(v.entry, 4), margin: s(v.margin, 2), funding: '0', isolated: false, liquidation_price: s(v.liq, 4), created_at: Date.now(), updated_at: Date.now() };
  });
}

function order(path, b, note) {
  const acct = L.account('pac', b.account);
  const coin = b.symbol;
  if (!MARKETS[coin]) return bad(`Unknown symbol ${coin}`);
  const amount = +b.amount;
  const lot = +MARKETS[coin].lot_size;
  if (Math.abs(Math.round(amount / lot) * lot - amount) > lot * 1e-6) return bad(`Amount ${b.amount} is not a multiple of lot size ${MARKETS[coin].lot_size}`);
  const buy = b.side === 'bid';
  const mark = mk.mid(coin, 'pac');
  if (path === '/orders/create') {
    const px = +b.price;
    const crosses = buy ? px >= mark : px <= mark;
    if (!crosses) {
      const oid = L.rest(acct, coin, buy, amount, px);
      return ok({ order_id: oid });
    }
  } else {
    const slip = +(b.slippage_percent || 0.5) / 100;
    note(`market ${b.side} ${b.amount} ${coin}, max slippage ${b.slippage_percent}%`);
    void slip;
  }
  const r = L.fill('pac', acct, coin, buy, amount, mark * (buy ? 1.0003 : 0.9997), !!b.reduce_only);
  if (!r.ok) return bad(r.error === 'insufficient_margin' ? 'Insufficient balance' : r.error);
  return ok({ order_id: L.db.oid++ });
}

register(BASE, {
  wsBase: 'wss://ws.pacifica.fi/ws',
  async http(method, path, q, headers, body, note) {
    if (method === 'GET') {
      if (path === '/info') return ok(Object.entries(MARKETS).map(([symbol, m]) => ({ symbol, ...m, min_tick: '0', max_tick: '1000000', max_order_size: '5000000', isolated_only: false, funding_rate: s(mk.funding(symbol), 7), next_funding_rate: s(mk.funding(symbol) * 1.05, 7) })));
      if (path === '/info/prices') return ok(Object.keys(MARKETS).map(prices));
      if (path === '/account') {
        const a = L.account('pac', q.account);
        const x = L.summary('pac', a);
        return ok({ balance: s(a.collateral, 6), fee_level: 0, account_equity: s(x.equity, 6), available_to_spend: s(x.free, 6), available_to_withdraw: s(x.free, 6), pending_balance: '0', total_margin_used: s(x.used, 6), positions_count: Object.keys(a.positions).length, orders_count: a.orders.length, stop_orders_count: 0, updated_at: Date.now() });
      }
      if (path === '/positions') return ok(positions(L.account('pac', q.account)));
      if (path === '/orders') return ok(L.account('pac', q.account).orders.map((o) => ({ order_id: o.oid, symbol: o.coin, side: o.buy ? 'bid' : 'ask', price: s(o.px, 4), initial_amount: s(o.sz, 5), filled_amount: '0', cancelled_amount: '0', order_type: 'limit', reduce_only: false, created_at: o.t })));
      if (path === '/kline') {
        const iv = { '1m': 60e3, '5m': 300e3, '15m': 900e3, '1h': 3600e3, '4h': 14400e3, '1d': 86400e3 }[q.interval] || 900e3;
        return ok(mk.candles(q.symbol, iv, +q.start_time, +(q.end_time || Date.now()), 'pac').map((b) => ({ t: b.t, T: b.T, s: q.symbol, i: q.interval, o: s(b.o, 4), c: s(b.c, 4), h: s(b.h, 4), l: s(b.l, 4), v: '812.3', n: 140 })));
      }
      return bad('Not found', 404);
    }
    const b = JSON.parse(body || '{}');
    if (!TYPES[path]) return bad('Not found', 404);
    const e = verify(path, b, note);
    if (e) return bad(e, 401);
    if (path === '/agent/bind') {
      db().agents[b.agent_wallet] = b.account;
      L.account('pac', b.account);
      L.save();
      return ok(null);
    }
    if (path === '/account/leverage') {
      const a = L.account('pac', b.account);
      a.lev[b.symbol] = +b.leverage;
      if (a.positions[b.symbol]) a.positions[b.symbol].lev = +b.leverage;
      L.save();
      return ok(null);
    }
    if (path === '/orders/cancel') {
      const a = L.account('pac', b.account);
      a.orders = a.orders.filter((o) => o.oid !== +b.order_id);
      return ok(null);
    }
    return order(path, b, note);
  },
  ws(path, sock) {
    let on = false;
    sock.onClientMessage = (text) => {
      const m = JSON.parse(text);
      if (m.method === 'subscribe' && m.params?.source === 'prices') on = true;
    };
    const t = setInterval(() => {
      if (!sock.open) return clearInterval(t);
      if (on) sock.push({ channel: 'prices', data: Object.keys(MARKETS).map(prices) }, true);
    }, 1000);
  },
});

function prices(symbol) {
  const m = mk.mid(symbol, 'pac');
  return { symbol, mark: s(m, 4), mid: s(m, 4), oracle: s(m * 0.9999, 4), funding: s(mk.funding(symbol), 7), next_funding: s(mk.funding(symbol) * 1.05, 7), open_interest: s(mk.openInterest(symbol) * 0.21, 2), volume_24h: s(mk.openInterest(symbol) * m * 0.7, 2), yesterday_price: s(mk.prevDay(symbol, 'pac'), 4), timestamp: Date.now() };
}
