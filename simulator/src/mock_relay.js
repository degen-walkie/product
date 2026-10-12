// Mock of the Degen Walkie relay (rooms, news, AI answers, the linking handshake).
// The real relay is a hosted service; this stand-in keeps the same endpoints.
// Room chatter is scripted and fills in live prices. The AI answers are preset
// templates over the position summary the device sends.
import * as mk from './market.js';
import * as L from './ledger.js';
import { register } from './net.js';

export const BASE = 'https://relay.degenwalkie.trade/v1';
const ROOMS = [
  { id: 'sol', name: 'SOL', coin: 'SOL', venue: 'hl', listening: 1204 },
  { id: 'hype', name: 'HYPE', coin: 'HYPE', venue: 'hl', listening: 688 },
  { id: 'btc', name: 'BTC', coin: 'BTC', venue: 'lit', listening: 2341 },
  { id: 'eth', name: 'ETH', coin: 'ETH', venue: 'pac', listening: 1019 },
  { id: 'world', name: 'WORLD', coin: '', venue: '', listening: 5132 },
  { id: '4471', name: '4471', coin: '', venue: '', listening: 6 },
];
const PEOPLE = [['@kaito', 'JA → EN'], ['@mara', 'ES → EN'], ['@lin', 'ZH → EN'], ['@jun', 'KO → EN'], ['@theo', 'EN'], ['@ana', 'PT → EN'], ['@degen_dad', 'EN']];
const LINES = [
  '{c} holding {px}. Bids stacked right under.',
  'Funding on {c} just flipped. Anyone adding here?',
  'Took profit on half, stop at entry.',
  'Same on Pacifica. Watching {lo} as support.',
  'Volume spiking on {c}. Something is moving.',
  'If {px} breaks I am out, too much leverage in here.',
  'Shorts getting squeezed, {c} up {chg} on the day.',
  '{c} looks heavy below {lo}. Careful with longs.',
  'Rolling into #{c} after this candle.',
  'Who is still long from {lo}? Holding or trimming?',
];
const WORLD = ['GM from Seoul. Who is trading CPI today?', 'BTC dominance creeping up again.', 'Anyone else on Pacifica? Fills are fast.', 'Rooms in Japanese are wild today.', 'Taking the weekend off the screen. Radio mode on.'];
const PRIVATE = ['Dinner at 8? Bringing the charts.', 'Closed my ETH, finally green.', 'Who is on the SOL long with me?', 'Room is quiet today.'];
const NEWS = [
  ['SOL', 'Funding flips negative on SOL-PERP across three venues'],
  ['BTC', 'Spot ETF inflows top $410M for the session'],
  ['ETH', '$38M of longs liquidated in 10 minutes'],
  ['HYPE', 'Open interest hits a new high'],
  ['MACRO', 'CPI in 45 minutes. Volatility expected'],
  ['SOL', 'Large SOL transfer to an exchange wallet'],
  ['BTC', 'Basis widens on quarterly futures'],
  ['ETH', 'ETH staking queue clears for the first time this month'],
  ['HYPE', 'Hyperliquid daily volume above $9B'],
  ['MACRO', 'Fed speaker: "no rush" on the next cut'],
];

const history = {};
const sockets = new Set();
let news = [];

const pick = (a) => a[Math.floor(Math.random() * a.length)];
const fmtPx = (v) => v.toLocaleString('en-US', { maximumFractionDigits: v >= 1000 ? 1 : 2, minimumFractionDigits: v >= 1000 ? 1 : 2 });

function line(room) {
  if (room.id === 'world') return pick(WORLD);
  if (!room.coin) return pick(PRIVATE);
  const venue = room.venue || 'hl';
  const px = mk.mid(room.coin, venue);
  const chg = ((px - mk.prevDay(room.coin, venue)) / mk.prevDay(room.coin, venue)) * 100;
  return pick(LINES).replaceAll('{c}', room.coin).replaceAll('{px}', fmtPx(px)).replaceAll('{lo}', fmtPx(px * 0.98)).replaceAll('{chg}', `${chg >= 0 ? '+' : ''}${chg.toFixed(1)}%`);
}

function post(roomId, who, lang, text, me = false) {
  const m = { type: 'msg', room: roomId, who, lang, text, me, ts: Date.now() };
  (history[roomId] = history[roomId] || []).push(m);
  if (history[roomId].length > 30) history[roomId].shift();
  for (const s of sockets) if (s.room === roomId) s.push(m, true);
}

export function startRelay() {
  for (const r of ROOMS) {
    history[r.id] = [];
    for (let i = 0; i < 4; i++) {
      const [who, lang] = pick(PEOPLE);
      history[r.id].push({ type: 'msg', room: r.id, who, lang, text: line(r), me: false, ts: Date.now() - (5 - i) * 60e3 });
    }
  }
  news = NEWS.slice(0, 6).map(([tag, text], i) => ({ tag, text, ts: Date.now() - [2, 9, 14, 31, 60, 95][i] * 60e3 }));
  const chatter = () => {
    const r = pick(ROOMS.slice(0, 5));
    const [who, lang] = pick(PEOPLE);
    post(r.id, who, lang, line(r));
    setTimeout(chatter, 3500 + Math.random() * 5000);
  };
  setTimeout(chatter, 3000);
  const breaking = () => {
    const [tag, text] = pick(NEWS);
    const item = { tag, text, ts: Date.now() };
    news.unshift(item);
    news = news.slice(0, 10);
    for (const s of sockets) s.push({ type: 'news', item });
    setTimeout(breaking, 45000 + Math.random() * 30000);
  };
  setTimeout(breaking, 30000);
  setInterval(() => {
    for (const r of ROOMS) {
      r.listening = Math.max(3, Math.round(r.listening * (1 + (Math.random() - 0.5) * 0.02)));
      for (const s of sockets) s.push({ type: 'listening', room: r.id, n: r.listening }, true);
    }
  }, 5000);
}

/* ---- preset AI answers over the device's position summary ---- */
const f2 = (v) => v.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 });
const sign = (v) => (v >= 0 ? '+' : '-') + f2(Math.abs(v));

function answer(q, ctx) {
  const text = q.toLowerCase();
  const pos = ctx.positions || [];
  const coin = ['btc', 'eth', 'sol', 'hype'].find((c) => text.includes(c));
  const target = coin ? pos.find((p) => p.coin.toLowerCase() === coin) : [...pos].sort((a, b) => Math.abs(b.size * b.mark) - Math.abs(a.size * a.mark))[0];
  if (/liquidat|liq\b|how far|risk/.test(text)) {
    if (!target) return coin ? `You have no ${coin.toUpperCase()} position open right now.` : 'You have no open positions, so nothing can be liquidated.';
    const dist = (Math.abs(target.mark - target.liq) / target.mark) * 100;
    const fund = target.market?.funding_h ?? 0;
    const pays = (fund > 0) === (target.size > 0);
    return `${target.coin} is ${f2(target.mark)}. Your ${target.size > 0 ? 'long' : 'short'} liquidates at ${f2(target.liq)}, ${dist.toFixed(1)}% away. Funding is ${pays ? 'costing you' : 'paying you'} ${(Math.abs(fund) * 100).toFixed(4)}% an hour.`;
  }
  if (/funding/.test(text)) {
    if (!pos.length) return 'No open positions, so funding is not touching you.';
    return pos.map((p) => {
      const fund = p.market?.funding_h ?? 0;
      const pays = (fund > 0) === (p.size > 0);
      return `${p.coin} on ${p.venue}: ${pays ? 'you pay' : 'you earn'} ${(Math.abs(fund) * 100).toFixed(4)}%/h`;
    }).join('. ') + '.';
  }
  if (/summar|book|overview|how am i|pnl|p&l/.test(text)) {
    if (!pos.length) return `Equity ${f2(ctx.equity || 0)} USDC and no open positions.`;
    const upnl = pos.reduce((a, p) => a + p.upnl, 0);
    const venues = new Set(pos.map((p) => p.venue)).size;
    const worst = [...pos].sort((a, b) => Math.abs(a.mark - a.liq) / a.mark - Math.abs(b.mark - b.liq) / b.mark)[0];
    const wd = (Math.abs(worst.mark - worst.liq) / worst.mark) * 100;
    return `Equity ${f2(ctx.equity || 0)} USDC across ${venues} venue${venues > 1 ? 's' : ''}. ${pos.length} positions, unrealized ${sign(upnl)}. Closest to liquidation: ${worst.coin} ${Math.round(worst.lev)}× on ${worst.venue}, ${wd.toFixed(1)}% away.`;
  }
  if (/price|where is|what is/.test(text) && coin) {
    const px = mk.mid(coin.toUpperCase());
    return `${coin.toUpperCase()} is ${f2(px)} on Hyperliquid right now.`;
  }
  if (/buy|sell|long|short|close/.test(text)) return 'I can read your book and the market, but I cannot place orders. Open the ticket and press the trade key yourself.';
  return 'I can answer about your positions, funding, liquidation distance and prices. I am read-only: only the trade key sends orders.';
}

/* ---- linking: the phone marks a session done after the main wallet approved ---- */
export function completeLink(session, venue, account, extra = '') {
  L.db.relay.links[session] = { status: 'done', venue, account, extra };
  L.save();
}

register(BASE, {
  wsBase: 'wss://relay.degenwalkie.trade/v1/ws',
  async http(method, path, q, headers, body, note) {
    if (path === '/rooms') return { status: 200, body: { rooms: ROOMS } };
    let m = path.match(/^\/rooms\/([^/]+)\/history$/);
    if (m) return { status: 200, body: { messages: history[m[1]] || [] } };
    if (path === '/news') return { status: 200, body: { items: news } };
    if (path === '/ask' && method === 'POST') {
      const b = JSON.parse(body || '{}');
      note(`AI saw: question + ${(b.positions || []).length} position summaries (no keys, no account access)`);
      await new Promise((r) => setTimeout(r, 500 + Math.random() * 600));
      return { status: 200, body: { answer: answer(b.q || '', b) } };
    }
    m = path.match(/^\/link\/([0-9a-f]+)$/);
    if (m) return { status: 200, body: L.db.relay.links[m[1]] || { status: 'pending' } };
    return { status: 404, body: { error: 'not found' } };
  },
  ws(path, s) {
    sockets.add(s);
    s.onClose = () => sockets.delete(s);
    s.onClientMessage = (text) => {
      const msg = JSON.parse(text);
      if (msg.op === 'join') s.room = msg.room;
      if (msg.op === 'say' && msg.text) {
        post(msg.room, '@you', msg.lang || 'EN', msg.text, true);
        const room = ROOMS.find((r) => r.id === msg.room);
        if (room && Math.random() < 0.8) {
          setTimeout(() => {
            const [who, lang] = pick(PEOPLE);
            const replies = room.coin
              ? [`Heard you. ${room.coin} at ${fmtPx(mk.mid(room.coin, room.venue || 'hl'))} now.`, 'Agreed, watching the same level.', 'Careful, funding is climbing.', 'Nice call. I am in too.']
              : ['Ha, see you there.', 'On my way.', 'Same here.'];
            post(msg.room, who, lang, pick(replies));
          }, 1800 + Math.random() * 2500);
        }
      }
    };
  },
});
