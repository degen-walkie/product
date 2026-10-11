// Mock of the Lighter API (https://mainnet.zklighter.elliot.ai/api/v1).
// Transactions arrive as form fields tx_type + tx_info, with integer-scaled sizes and
// prices and a per-API-key nonce. Real Lighter verifies a Schnorr/Poseidon2 signature;
// the firmware has not ported that signer yet and sends an Ed25519 stand-in, which
// this mock verifies against the registered API key.
import * as mk from './market.js';
import * as L from './ledger.js';
import { register } from './net.js';
import { b64decode, edVerify, short, unhex } from './util.js';

export const BASE = 'https://mainnet.zklighter.elliot.ai/api/v1';
const BOOKS = [
  { symbol: 'ETH', market_id: 0, size: 4, price: 2, min_base: '0.0050' },
  { symbol: 'BTC', market_id: 1, size: 5, price: 1, min_base: '0.00020' },
  { symbol: 'SOL', market_id: 2, size: 3, price: 3, min_base: '0.050' },
];
const TX = { CHANGE_PUB_KEY: 8, CREATE_ORDER: 14, UPDATE_LEVERAGE: 20 };
const db = () => L.db.lit;
const s = (n, d) => (+n).toFixed(d);
const byId = (id) => BOOKS.find((b) => b.market_id === +id);
const ok = (extra) => ({ status: 200, body: { code: 200, ...extra } });
const fail = (code, message, http = 400) => ({ status: http, body: { code, message } });

export function registerKey(acct, idx, pubHex, l1) {
  db().keys[`${acct}:${idx}`] = pubHex.replace(/^0x/, '');
  db().nonces[`${acct}:${idx}`] = 0;
  const a = L.account('lit', String(acct));
  a.l1 = l1;
  L.save();
}

function accountJson(idx) {
  const a = L.account('lit', String(idx));
  const x = L.summary('lit', a);
  return {
    code: 1, account_type: 0, index: +idx, account_index: +idx, l1_address: a.l1 || '', status: 1, collateral: s(a.collateral, 6),
    available_balance: s(x.free, 6), total_asset_value: s(x.equity, 6), cross_asset_value: s(x.equity, 6),
    positions: Object.keys(a.positions).map((coin) => {
      const v = L.positionView('lit', a, coin);
      const b = BOOKS.find((k) => k.symbol === coin);
      return { market_id: b.market_id, symbol: coin, initial_margin_fraction: s(100 / v.lev, 2), open_order_count: 0, sign: v.szi > 0 ? 1 : -1, position: s(Math.abs(v.szi), b.size), avg_entry_price: s(v.entry, b.price), position_value: s(v.value, 6), unrealized_pnl: s(v.upnl, 6), realized_pnl: '0.000000', liquidation_price: s(v.liq, b.price), margin_mode: 0, allocated_margin: '0.000000' };
    }),
  };
}

function sendTx(form, note) {
  const type = +form.get('tx_type');
  let tx;
  try { tx = JSON.parse(form.get('tx_info')); } catch { return fail(21100, 'invalid tx info'); }
  if (type === TX.CHANGE_PUB_KEY) {
    // Signed by the L1 wallet in the real flow; the phone in the simulator does this.
    registerKey(tx.AccountIndex, tx.ApiKeyIndex, tx.PubKey, tx.L1Address);
    note(`API key #${tx.ApiKeyIndex} registered for account ${tx.AccountIndex} by its L1 wallet`);
    return ok({ message: '{"ratelimit": "didn\'t use volume quota"}', tx_hash: randHash() });
  }
  const id = `${tx.AccountIndex}:${tx.ApiKeyIndex}`;
  const pub = db().keys[id];
  if (!pub) { note(`✗ no API key #${tx.ApiKeyIndex} on account ${tx.AccountIndex}`); return fail(21109, 'api key not found'); }
  const { Sig, ...unsigned } = tx;
  if (!edVerify(b64decode(Sig || ''), JSON.stringify(unsigned), unhex(pub))) { note('✗ signature check failed'); return fail(21120, 'invalid signature'); }
  note(`✓ stand-in signature by API key #${tx.ApiKeyIndex} (${short(pub, 6, 4)}) · Lighter itself uses Schnorr/Poseidon2`);
  if (tx.Nonce !== db().nonces[id]) return fail(21104, `invalid nonce, expected ${db().nonces[id]}`);
  if (tx.ExpiredAt < Date.now()) return fail(21105, 'transaction expired');
  db().nonces[id]++;
  const acct = L.account('lit', String(tx.AccountIndex));
  const book = byId(tx.MarketIndex);
  if (!book) return fail(21600, 'invalid market index');
  if (type === TX.UPDATE_LEVERAGE) {
    const lev = Math.round(10000 / tx.InitialMarginFraction);
    acct.lev[book.symbol] = lev;
    if (acct.positions[book.symbol]) acct.positions[book.symbol].lev = lev;
    note(`leverage ${book.symbol} → ${lev}× (initial margin fraction ${tx.InitialMarginFraction / 100}%)`);
    L.save();
    return ok({ message: '{"ratelimit": "didn\'t use volume quota"}', tx_hash: randHash() });
  }
  if (type === TX.CREATE_ORDER) {
    const sz = tx.BaseAmount / 10 ** book.size;
    const px = tx.Price / 10 ** book.price;
    const buy = !tx.IsAsk;
    const mark = mk.mid(book.symbol, 'lit');
    note(`${buy ? 'buy' : 'sell'} ${sz} ${book.symbol} ${tx.Type === 1 ? 'market' : 'limit'} @ ${px} (BaseAmount ${tx.BaseAmount}, Price ${tx.Price})`);
    if (sz < +book.min_base && !tx.ReduceOnly) return fail(21706, 'order size below minimum');
    const crosses = buy ? px >= mark : px <= mark;
    if (!crosses) {
      if (tx.Type === 1) return ok({ message: 'order cancelled: price outside slippage', tx_hash: randHash() });
      L.rest(acct, book.symbol, buy, sz, px);
      return ok({ message: '{"ratelimit": "didn\'t use volume quota"}', tx_hash: randHash() });
    }
    const r = L.fill('lit', acct, book.symbol, buy, sz, mark, !!tx.ReduceOnly);
    if (!r.ok) return fail(21701, r.error === 'insufficient_margin' ? 'not enough margin' : r.error);
    return ok({ message: '{"ratelimit": "didn\'t use volume quota"}', tx_hash: randHash() });
  }
  return fail(21000, `tx type ${type} not supported in the simulator`);
}

const randHash = () => Array.from(crypto.getRandomValues(new Uint8Array(32)), (x) => x.toString(16).padStart(2, '0')).join('');

register(BASE, {
  async http(method, path, q, headers, body, note) {
    if (method === 'POST' && path === '/sendTx') return sendTx(new URLSearchParams(body || ''), note);
    if (path === '/orderBooks') {
      return ok({ order_books: BOOKS.map((b) => ({ symbol: b.symbol, market_id: b.market_id, status: 'active', taker_fee: '0.0000', maker_fee: '0.0000', liquidation_fee: '1.0000', min_base_amount: b.min_base, min_quote_amount: '10.000000', supported_size_decimals: b.size, supported_price_decimals: b.price, supported_quote_decimals: 6 })) });
    }
    if (path === '/orderBookDetails') {
      const list = q.market_id !== undefined ? [byId(q.market_id)] : BOOKS;
      return ok({ order_book_details: list.map((b) => {
        const m = mk.mid(b.symbol, 'lit'), p = mk.prevDay(b.symbol, 'lit');
        return { symbol: b.symbol, market_id: b.market_id, status: 'active', size_decimals: b.size, price_decimals: b.price, last_trade_price: +s(m, b.price), daily_price_change: +(((m - p) / p) * 100).toFixed(4), daily_price_low: +s(Math.min(m, p) * 0.99, b.price), daily_price_high: +s(Math.max(m, p) * 1.01, b.price), open_interest: mk.openInterest(b.symbol) * 0.12, daily_base_token_volume: 12000, daily_quote_token_volume: 12000 * m };
      }) });
    }
    if (path === '/account') return ok({ total: 1, accounts: [accountJson(q.value)] });
    if (path === '/nextNonce') return ok({ nonce: db().nonces[`${q.account_index}:${q.api_key_index}`] ?? 0 });
    if (path === '/candlesticks') {
      const b = byId(q.market_id);
      const iv = { '1m': 60e3, '5m': 300e3, '15m': 900e3, '1h': 3600e3, '4h': 14400e3, '1d': 86400e3 }[q.resolution] || 900e3;
      return ok({ resolution: q.resolution, candlesticks: mk.candles(b.symbol, iv, +q.start_timestamp, +q.end_timestamp, 'lit').map((c) => ({ timestamp: c.t, open: +s(c.o, b.price), high: +s(c.h, b.price), low: +s(c.l, b.price), close: +s(c.c, b.price), volume0: 120.5, volume1: 120.5 * c.c })) });
    }
    return fail(404, 'not found', 404);
  },
});
