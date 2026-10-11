// Mock of the Hyperliquid API (https://api.hyperliquid.xyz): /info, /exchange and the
// WebSocket. Signatures are checked the way Hyperliquid checks them: L1 actions are
// msgpack-hashed into a "phantom agent" and signed with EIP-712 (chainId 1337); user
// actions (approveAgent, withdraw3) use the HyperliquidSignTransaction domain.
import { encode as msgpack } from '@msgpack/msgpack';
import * as mk from './market.js';
import * as L from './ledger.js';
import { register } from './net.js';
import { concat, eip712Digest, keccak, recoverAddress, short, hex } from './util.js';

export const BASE = 'https://api.hyperliquid.xyz';
const NAMES = ['BTC', 'ETH', 'ATOM', 'MATIC', 'DYDX', 'SOL', 'AVAX', 'BNB', 'APE', 'OP', 'LTC', 'ARB', 'DOGE', 'INJ', 'SUI', 'HYPE'];
const SZ_DEC = { BTC: 5, ETH: 4, SOL: 2, HYPE: 2 };
const MAX_LEV = { BTC: 40, ETH: 25, SOL: 20, HYPE: 10 };
const ZERO = '0x0000000000000000000000000000000000000000';
const db = () => L.db.hl;
const str = (n, d = 6) => (+n).toFixed(d).replace(/\.?0+$/, '') || '0';

function universe() {
  return NAMES.map((name) => ({ name, szDecimals: SZ_DEC[name] ?? 2, maxLeverage: MAX_LEV[name] ?? 10, onlyIsolated: false }));
}

function ctx(name) {
  if (!mk.COINS.includes(name)) return { funding: '0.0000125', openInterest: '0', prevDayPx: '1.0', dayNtlVlm: '0', markPx: '1.0', midPx: '1.0', oraclePx: '1.0' };
  const m = mk.mid(name, 'hl');
  return {
    funding: str(mk.funding(name), 8), openInterest: str(mk.openInterest(name), 2), prevDayPx: str(mk.prevDay(name, 'hl'), 4),
    dayNtlVlm: str(mk.openInterest(name) * m * 3.1, 2), premium: '0.0003', oraclePx: str(m * 0.9998, 4), markPx: str(m, 4), midPx: str(m, 4),
  };
}

function state(user) {
  const a = L.account('hl', user.toLowerCase());
  const s = L.summary('hl', a);
  const assetPositions = Object.keys(a.positions).map((coin) => {
    const v = L.positionView('hl', a, coin);
    return {
      type: 'oneWay',
      position: {
        coin, szi: str(v.szi, SZ_DEC[coin]), entryPx: str(v.entry, 4), positionValue: str(v.value, 2), unrealizedPnl: str(v.upnl, 2),
        returnOnEquity: str(v.roe, 4), liquidationPx: str(v.liq, 4), marginUsed: str(v.margin, 2),
        leverage: { type: 'cross', value: v.lev }, maxLeverage: MAX_LEV[coin], cumFunding: { allTime: '0.0', sinceOpen: '0.0', sinceChange: '0.0' },
      },
    };
  });
  return {
    marginSummary: { accountValue: str(s.equity, 2), totalNtlPos: str(s.ntl, 2), totalRawUsd: str(a.collateral, 2), totalMarginUsed: str(s.used, 2) },
    crossMarginSummary: { accountValue: str(s.equity, 2), totalNtlPos: str(s.ntl, 2), totalRawUsd: str(a.collateral, 2), totalMarginUsed: str(s.used, 2) },
    crossMaintenanceMarginUsed: str(s.used * 0.5, 2), withdrawable: str(s.free, 2), assetPositions, time: Date.now(),
  };
}

function info(body) {
  switch (body.type) {
    case 'meta': return { universe: universe() };
    case 'metaAndAssetCtxs': return [{ universe: universe() }, NAMES.map(ctx)];
    case 'allMids': return Object.fromEntries(mk.COINS.map((c) => [c, str(mk.mid(c, 'hl'), 4)]));
    case 'clearinghouseState': return state(body.user);
    case 'openOrders':
      return L.account('hl', body.user.toLowerCase()).orders.map((o) => ({ coin: o.coin, side: o.buy ? 'B' : 'A', limitPx: str(o.px, 4), sz: str(o.sz, 5), oid: o.oid, timestamp: o.t, origSz: str(o.sz, 5) }));
    case 'extraAgents':
      return Object.entries(db().agents).filter(([, v]) => v.user === body.user.toLowerCase()).map(([address, v]) => ({ address, name: v.name, validUntil: v.validUntil }));
    case 'candleSnapshot': {
      const { coin, interval, startTime, endTime } = body.req;
      const iv = { '1m': 60e3, '5m': 300e3, '15m': 900e3, '1h': 3600e3, '4h': 14400e3, '1d': 86400e3 }[interval] || 900e3;
      return mk.candles(coin, iv, startTime, endTime, 'hl').map((b) => ({ t: b.t, T: b.T, s: coin, i: interval, o: str(b.o, 4), c: str(b.c, 4), h: str(b.h, 4), l: str(b.l, 4), v: '1234.5', n: 321 }));
    }
    default: return null;
  }
}

/* ---- signatures ---- */

export function l1Digest(action, nonce, vault) {
  const nb = new Uint8Array(8);
  new DataView(nb.buffer).setBigUint64(0, BigInt(nonce));
  const tail = vault ? concat(new Uint8Array([1]), Uint8Array.from(Buffer_from_hex(vault))) : new Uint8Array([0]);
  const connectionId = keccak(concat(msgpack(action), nb, tail));
  const digest = eip712Digest({ name: 'Exchange', version: '1', chainId: 1337, verifyingContract: ZERO }, 'Agent',
    [['string', 'source'], ['bytes32', 'connectionId']], { source: 'a', connectionId });
  return { digest, connectionId };
}
function Buffer_from_hex(h) { return h.replace(/^0x/, '').match(/../g).map((x) => parseInt(x, 16)); }

export const USER_TYPES = {
  approveAgent: ['HyperliquidTransaction:ApproveAgent', [['string', 'hyperliquidChain'], ['address', 'agentAddress'], ['string', 'agentName'], ['uint64', 'nonce']]],
  withdraw3: ['HyperliquidTransaction:Withdraw', [['string', 'hyperliquidChain'], ['string', 'destination'], ['string', 'amount'], ['uint64', 'time']]],
};
export function userDigest(action) {
  const [primary, fields] = USER_TYPES[action.type];
  return eip712Digest({ name: 'HyperliquidSignTransaction', version: '1', chainId: parseInt(action.signatureChainId, 16), verifyingContract: ZERO }, primary, fields, action);
}

/* ---- price / size rules ---- */
function validPx(px, coin) {
  const s = String(px);
  const dec = (s.split('.')[1] || '').length;
  const sig = s.replace('.', '').replace(/^0+/, '').replace(/0+$/, '').length;
  return dec <= 6 - (SZ_DEC[coin] ?? 2) && (sig <= 5 || !s.includes('.'));
}

function exchange(body, note) {
  const { action, nonce, signature } = body;
  const err = (msg) => ({ status: 'err', response: msg });
  if (USER_TYPES[action.type]) {
    const signer = recoverAddress(userDigest(action), signature).toLowerCase();
    note(`EIP-712 user action ${action.type} recovered signer ${short(signer)}`);
    if (action.type === 'approveAgent') {
      db().agents[action.agentAddress.toLowerCase()] = { user: signer, name: action.agentName || '', validUntil: Date.now() + 180 * 86400e3 };
      L.account('hl', signer);
      L.save();
      note(`agent ${short(action.agentAddress)} approved for ${short(signer)} (trade-only)`);
      return { status: 'ok', response: { type: 'default' } };
    }
    if (db().agents[signer] || !db().accounts[signer]) {
      note(`✗ ${short(signer)} is not an account owner. Agent keys cannot sign ${action.type}.`);
      return err(`User or API Wallet ${signer} does not exist.`);
    }
    return { status: 'ok', response: { type: 'default' } };
  }

  const { digest, connectionId } = l1Digest(action, nonce, body.vaultAddress);
  let signer;
  try { signer = recoverAddress(digest, signature).toLowerCase(); } catch { return err('Invalid signature.'); }
  note(`msgpack(action)+nonce → connectionId ${short('0x' + hex(connectionId), 8, 6)}`);
  const agent = db().agents[signer];
  if (!agent) {
    note(`✗ recovered ${short(signer)}: not an approved agent`);
    return err(`User or API Wallet ${signer} does not exist.`);
  }
  note(`✓ EIP-712 phantom-agent signature recovered ${short(signer)} → agent of ${short(agent.user)}`);
  if (Math.abs(nonce - Date.now()) > 86400e3) return err('Invalid nonce: must be within one day of the current time.');
  const seen = (db().nonces[signer] = db().nonces[signer] || []);
  if (seen.includes(nonce)) return err('Invalid nonce: duplicate nonce.');
  seen.push(nonce);
  if (seen.length > 100) seen.shift();
  const acct = L.account('hl', agent.user);

  if (action.type === 'updateLeverage') {
    const coin = NAMES[action.asset];
    acct.lev[coin] = action.leverage;
    if (acct.positions[coin]) acct.positions[coin].lev = action.leverage;
    L.save();
    return { status: 'ok', response: { type: 'default' } };
  }
  if (action.type === 'cancel') {
    acct.orders = acct.orders.filter((o) => !action.cancels.some((c) => c.o === o.oid));
    return { status: 'ok', response: { type: 'cancel', data: { statuses: action.cancels.map(() => 'success') } } };
  }
  if (action.type === 'order') {
    const statuses = action.orders.map((o) => {
      const coin = NAMES[o.a];
      if (!mk.COINS.includes(coin)) return { error: `Asset ${o.a} is not tradable in the simulator.` };
      if (!validPx(o.p, coin)) return { error: 'Order has invalid price.' };
      const px = +o.p, sz = +o.s, buy = o.b, mark = mk.mid(coin, 'hl');
      if (sz <= 0) return { error: 'Order has zero size.' };
      if (sz * px < 10 && !o.r) return { error: 'Order must have minimum value of $10.' };
      const crosses = buy ? px >= mark : px <= mark;
      const tif = o.t?.limit?.tif || 'Gtc';
      if (!crosses) {
        if (tif === 'Ioc') return { error: `Order could not immediately match against any resting orders. asset=${o.a}` };
        const lev = acct.lev[coin] || 5;
        if ((sz * px) / lev > L.summary('hl', acct).free) return { error: `Insufficient margin to place order. asset=${o.a}` };
        return { resting: { oid: L.rest(acct, coin, buy, sz, px) } };
      }
      const fillPx = mark * (buy ? 1.0002 : 0.9998);
      const r = L.fill('hl', acct, coin, buy, sz, fillPx, o.r);
      if (!r.ok) return { error: r.error === 'insufficient_margin' ? `Insufficient margin to place order. asset=${o.a}` : r.error };
      return { filled: { totalSz: str(r.sz, SZ_DEC[coin]), avgPx: str(r.px, 4), oid: L.db.oid++ } };
    });
    return { status: 'ok', response: { type: 'order', data: { statuses } } };
  }
  return err(`Unsupported action in the simulator: ${action.type}`);
}

register(BASE, {
  wsBase: 'wss://api.hyperliquid.xyz/ws',
  async http(method, path, q, headers, body, note) {
    const b = body ? JSON.parse(body) : {};
    if (path === '/info') {
      const r = info(b);
      return r === null ? { status: 422, body: 'Failed to deserialize the JSON body into the target type' } : { status: 200, body: r };
    }
    if (path === '/exchange') return { status: 200, body: exchange(b, note) };
    return { status: 404, body: 'Not Found' };
  },
  ws(path, s) {
    const subs = new Set();
    s.onClientMessage = (text) => {
      const m = JSON.parse(text);
      if (m.method === 'subscribe') {
        subs.add(m.subscription.type);
        s.push({ channel: 'subscriptionResponse', data: m });
      }
    };
    const t = setInterval(() => {
      if (!s.open) return clearInterval(t);
      if (subs.has('allMids')) s.push({ channel: 'allMids', data: { mids: info({ type: 'allMids' }) } }, true);
    }, 1000);
  },
});
