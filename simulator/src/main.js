// Simulator page. Two modes:
//   Demo  – scripted walkthroughs on the real firmware against in-page mocks.
//   Real  – you drive the device; it trades on Hyperliquid / Pacifica testnet (or mainnet).
import * as mk from './market.js';
import * as L from './ledger.js';
import { onLog, setReal } from './net.js';
import './mock_hyperliquid.js';
import './mock_pacifica.js';
import './mock_lighter.js';
import { startRelay, BASE as MOCK_RELAY } from './mock_relay.js';
import { approveQr, provisionDemo, tryWithdrawWithDeviceKey } from './phone.js';
import { createHost, IN, storeKey } from './host.js';
import { createPlayer, DEMOS } from './demo.js';
import * as W from './wallet.js';
import { short } from './util.js';

const $ = (id) => document.getElementById(id);
const params = new URLSearchParams(location.search);
const PAGE = 'dw.page.';
const get = (k, d) => localStorage.getItem(PAGE + k) || d;
const set = (k, v) => localStorage.setItem(PAGE + k, v);

const MODE = params.get('mode') || get('mode', 'demo');            // demo | real
const NET = MODE === 'real' ? get('net', 'testnet') : 'demo';         // demo | testnet | mainnet
const NETU = NET.toUpperCase();
const defaultRelay = /^(localhost|127\.)/.test(location.hostname) ? 'http://localhost:8787/v1' : 'https://relay-demo.degenwalkie.trade/v1';
const RELAY = get('relay', defaultRelay);
const LINK_URL = new URL('link.html', import.meta.url).href;
const PLAY = params.get('play');

setReal(MODE === 'real');
document.body.classList.add(`mode-${MODE}`);

/* ---------------- header switches ---------------- */
document.querySelectorAll('#mode-switch button').forEach((b) => {
  b.classList.toggle('on', b.dataset.mode === MODE);
  b.addEventListener('click', () => { if (b.dataset.mode !== MODE) { set('mode', b.dataset.mode); location.href = location.pathname; } });
});
$('net-switch').hidden = MODE !== 'real';
document.querySelectorAll('#net-switch button').forEach((b) => {
  b.classList.toggle('on', b.dataset.net === NET);
  b.addEventListener('click', () => {
    if (b.dataset.net === NET) return;
    if (b.dataset.net === 'mainnet') { $('mainnet-dlg').showModal(); return; }
    set('net', 'testnet');
    location.reload();
  });
});
$('mainnet-ok').addEventListener('change', (e) => { $('mainnet-go').disabled = !e.target.checked; });
$('mainnet-cancel').addEventListener('click', () => $('mainnet-dlg').close());
$('mainnet-go').addEventListener('click', () => { set('net', 'mainnet'); location.reload(); });
if (MODE === 'real') {
  const bn = $('banner');
  bn.hidden = false;
  bn.className = `banner ${NET}`;
  bn.textContent = NET === 'mainnet'
    ? 'MAINNET · REAL MONEY · Degen Walkie is in development and unaudited. Use a fresh account and small amounts.'
    : 'TESTNET · Hyperliquid and Pacifica testnets · test USDC only · Lighter is coming soon';
}

/* ---------------- screen ---------------- */
const canvas = $('screen');
const ctx = canvas.getContext('2d');
const image = ctx.createImageData(320, 480);
const pixels = new Uint32Array(image.data.buffer);
let currentQr = '';
let listening = false;
let recog = null, recogText = '';

const ui = {
  pixels,
  paint() { ctx.putImageData(image, 0, 0); },
  led(color, mode) { $('light').className = `light ${color} ${color === 'off' ? '' : mode}`; },
  vibrate() { const d = $('device'); d.classList.remove('buzz'); void d.offsetWidth; d.classList.add('buzz'); },
  backlight(on) { $('off').classList.toggle('on', !on); },
  qr(payload) { currentQr = payload; renderScan(); },
  stored() { if (MODE === 'real') renderChecklist(); },
  listen(on) {
    listening = on;
    $('ptt').classList.toggle('on', on);
    if (on) {
      recogText = '';
      if ($('use-mic').checked && recog) { try { recog.start(); } catch { /* running */ } }
      return Promise.resolve();
    }
    return new Promise((resolve) => {
      const typed = () => $('say').value.trim() || $('say').placeholder;
      if ($('use-mic').checked && recog) {
        recog.onend = () => resolve(recogText || typed());
        try { recog.stop(); } catch { resolve(typed()); }
        setTimeout(() => resolve(recogText || typed()), 2500);
      } else setTimeout(() => resolve(typed()), 300);
    });
  },
};

const SR = window.SpeechRecognition || window.webkitSpeechRecognition;
if (SR) {
  recog = new SR();
  recog.lang = 'en-US';
  recog.onresult = (e) => { recogText = Array.from(e.results).map((r) => r[0].transcript).join(' '); $('say').value = recogText; };
} else $('use-mic').disabled = true;

/* ---------------- effects (what is being pressed) ---------------- */
const KEY_EL = { wheel: 'k-wheel', trade: 'k-trade', power: 'k-power' };
const BAR_IN = (which, label) => ({ wheel: label.includes('▼') ? 1 : label.includes('▲') ? 0 : 2, trade: 3, power: label.includes('Hold') ? 7 : 4 })[which];
const fx = {
  key(which, label, ms = 650) {
    const c = document.createElement('div');
    c.className = 'callout ' + (which === 'power' ? 'left' : which === 'ptt' ? 'ptt' : 'right');
    c.textContent = label;
    if (KEY_EL[which]) {
      const k = $(KEY_EL[which]);
      k.classList.add('hl');
      setTimeout(() => k.classList.remove('hl'), ms);
      c.style.top = `${k.offsetTop + k.offsetHeight / 2}px`;
    }
    if (which === 'ptt') $('ptt').classList.add('on');
    $('callouts').appendChild(c);
    setTimeout(() => { c.remove(); if (which === 'ptt') $('ptt').classList.remove('on'); }, ms);
    const bar = document.querySelector(`#bar button[data-in="${BAR_IN(which, label)}"]`);
    if (bar) { bar.classList.add('down'); setTimeout(() => bar.classList.remove('down'), Math.min(ms, 500)); }
  },
  tap(x, y, label) {
    const f = $('fx'), s = f.clientWidth / 320;
    const t = document.createElement('div');
    t.className = 'tapfx';
    t.style.left = `${x * s}px`; t.style.top = `${y * s}px`;
    f.appendChild(t);
    setTimeout(() => t.remove(), 800);
    if (label) {
      const l = document.createElement('div');
      l.className = 'taplabel';
      l.textContent = `tap ${label}`;
      l.style.left = `${x * s}px`; l.style.top = `${y * s}px`;
      f.appendChild(l);
      setTimeout(() => l.remove(), 1200);
    }
  },
  swipe(x1, y1, x2, y2) {
    const f = $('fx'), s = f.clientWidth / 320;
    const t = document.createElement('div');
    t.className = 'swipefx';
    const dx = (x2 - x1) * s, dy = (y2 - y1) * s;
    t.style.left = `${x1 * s}px`; t.style.top = `${y1 * s}px`;
    t.style.width = `${Math.hypot(dx, dy)}px`;
    t.style.transform = `rotate(${Math.atan2(dy, dx)}rad)`;
    f.appendChild(t);
    setTimeout(() => t.remove(), 950);
  },
  phone() { showCaption('Your phone: approve the device key in your main wallet…'); },
};

/* ---------------- boot ---------------- */
function demoWipe() {
  for (const k of Object.keys(localStorage))
    if (k.startsWith('dw.fw.demo.') || k === 'dw.mock' || k === 'dw.demo.provisioned' || k === 'dw.phone') localStorage.removeItem(k);
  L.reset();
}

const playDemoDef = PLAY && DEMOS.find((d) => d.id === PLAY);
const host = createHost(ui, {
  ns: NET,
  relayUrl: MODE === 'real' ? RELAY : MOCK_RELAY,   // the demo's relay is the in-page mock
  linkUrl: MODE === 'real' ? LINK_URL : null,
  bootOff: MODE === 'real' || !!(playDemoDef && playDemoDef.off),
});

async function boot() {
  if (MODE === 'demo') {
    if (playDemoDef) demoWipe();
    await mk.initMarket();
    setInterval(() => { mk.tick(1); L.matchResting(); }, 1000);
    startRelay();
    if (!playDemoDef || !playDemoDef.fresh) {
      await provisionDemo();
      localStorage.setItem(storeKey('demo', 'setup.done'), '1');
      localStorage.setItem(storeKey('demo', 'lang'), 'EN');
    }
  }
  const mod = await window.createWalkie({ dwHost: host, locateFile: (f) => new URL(`fw/${f}`, import.meta.url).href, print: () => {}, printErr: (e) => console.warn(e) });
  host.attach(mod);
  $('boot').classList.add('done');
  if (playDemoDef) setTimeout(() => playDemo(PLAY), playDemoDef.off ? 600 : 2300);
}
boot().catch((e) => { console.error(e); $('boot').textContent = 'Could not start the firmware: ' + e.message; });

/* ---------------- input ---------------- */
let playing = false;
const send = (code) => { if (!playing) host.input(code); };
document.querySelectorAll('#bar [data-in]').forEach((b) => b.addEventListener('click', () => send(+b.dataset.in)));
$('k-trade').addEventListener('click', () => send(IN.TRADE));
$('k-wheel').addEventListener('click', () => send(IN.PRESS));
let pwrTimer = null, pwrHeld = false;
$('k-power').addEventListener('pointerdown', () => { pwrHeld = false; pwrTimer = setTimeout(() => { pwrHeld = true; send(IN.POWER_HOLD); }, 1200); });
$('k-power').addEventListener('pointerup', () => { clearTimeout(pwrTimer); if (!pwrHeld) send(IN.POWER); });
let wheelAcc = 0;
const onWheel = (e) => {
  e.preventDefault();
  wheelAcc += e.deltaY;
  while (Math.abs(wheelAcc) >= 60) { send(wheelAcc > 0 ? IN.DOWN : IN.UP); wheelAcc -= Math.sign(wheelAcc) * 60; }
};
$('k-wheel').addEventListener('wheel', onWheel, { passive: false });
canvas.addEventListener('wheel', onWheel, { passive: false });

const ptt = $('ptt');
const pttDown = () => { if (!listening && !playing) host.input(IN.PTT_DOWN); };
const pttUp = () => { if (listening && !playing) host.input(IN.PTT_UP); };
ptt.addEventListener('pointerdown', (e) => { ptt.setPointerCapture(e.pointerId); pttDown(); });
ptt.addEventListener('pointerup', pttUp);
ptt.addEventListener('pointercancel', pttUp);

window.addEventListener('keydown', (e) => {
  if (e.target instanceof HTMLInputElement || e.target instanceof HTMLSelectElement || playing) return;
  const k = e.key;
  if (k === 'ArrowUp') send(IN.UP);
  else if (k === 'ArrowDown') send(IN.DOWN);
  else if (k === 'Escape' || k === 'Backspace') send(IN.PRESS);
  else if (k === 'Enter' || k === ' ') send(IN.TRADE);
  else if (k === 'P') send(IN.POWER_HOLD);
  else if (k === 'p') send(IN.POWER);
  else if ((k === 'm' || k === 'M') && !e.repeat) pttDown();
  else return;
  e.preventDefault();
});
window.addEventListener('keyup', (e) => { if (e.key === 'm' || e.key === 'M') pttUp(); });

function toDevice(e) {
  const r = canvas.getBoundingClientRect();
  return [Math.round(((e.clientX - r.left) / r.width) * 320), Math.round(((e.clientY - r.top) / r.height) * 480)];
}
let touching = false;
canvas.addEventListener('pointerdown', (e) => { if (playing) return; touching = true; canvas.setPointerCapture(e.pointerId); host.touch(...toDevice(e), true); });
canvas.addEventListener('pointermove', (e) => { if (touching) host.touch(...toDevice(e), true); });
const lift = (e) => { if (touching) { touching = false; host.touch(...toDevice(e), false); } };
canvas.addEventListener('pointerup', lift);
canvas.addEventListener('pointercancel', lift);

/* ---------------- demo mode ---------------- */
function showCaption(t) { const c = $('caption'); c.hidden = false; c.textContent = t; }
const player = createPlayer({
  host, fx,
  onCaption: showCaption,
  onStep: (i) => {
    const demo = DEMOS.find((d) => d.id === $('demo-sel').value);
    const caps = demo ? demo.steps.map((s, j) => [s, j]).filter(([s]) => s[0] === 'cap') : [];
    let now = -1;
    caps.forEach(([, j], k) => { if (j <= i) now = k; });
    [...$('steps').children].forEach((li, k) => { li.className = i < 0 ? 'done' : k < now ? 'done' : k === now ? 'now' : ''; });
  },
  approve: async () => { if (currentQr) await approveQr(currentQr); },
  setMic: (t) => { $('say').value = t; },
});

async function playDemo(id) {
  const demo = DEMOS.find((d) => d.id === id);
  if (!demo) return;
  playing = true;
  $('lock').hidden = false;
  $('play').textContent = '■ Stop';
  const done = await player.play(demo);
  playing = false;
  $('lock').hidden = true;
  $('play').textContent = '↻ Play again';
  if (done) showCaption(`${demo.title}: done. Pick another demo, or switch to Real.`);
}

if (MODE === 'demo') {
  $('real-panel').hidden = true;
  const sel = $('demo-sel');
  sel.innerHTML = DEMOS.map((d) => `<option value="${d.id}">${d.title}</option>`).join('');
  sel.value = PLAY || 'open';
  const renderDemo = () => {
    const d = DEMOS.find((x) => x.id === sel.value);
    $('demo-blurb').textContent = d.blurb;
    $('steps').innerHTML = d.steps.filter((s) => s[0] === 'cap').map((s) => `<li>${s[1]}</li>`).join('');
  };
  sel.addEventListener('change', () => { renderDemo(); $('play').textContent = '▶ Play'; });
  renderDemo();
  $('play').addEventListener('click', () => {
    if (playing) { player.stop(); return; }
    location.href = `${location.pathname}?mode=demo&play=${sel.value}`;   // a clean device for every run
  });
  showCaption(PLAY ? 'Starting…' : 'Pick a demo on the right and press Play.');
  $('attack').addEventListener('click', async () => {
    const out = $('attack-out');
    out.hidden = false;
    out.textContent = 'Signing withdraw3 with the device key…';
    const r = await tryWithdrawWithDeviceKey();
    out.textContent = r.text.startsWith('{')
      ? `Hyperliquid (mock) answered:\n${r.text}\n\nAn agent key can sign orders, but withdrawals must be signed by the account owner.`
      : r.text;
  });
} else {
  $('demo-panel').hidden = true;
  $('real-panel').hidden = false;
}

/* ---------------- real mode ---------------- */
document.querySelectorAll('.tabs button').forEach((b) => b.addEventListener('click', () => {
  document.querySelectorAll('.tabs button').forEach((x) => x.classList.toggle('on', x === b));
  document.querySelectorAll('.tab').forEach((t) => { t.hidden = t.id !== `tab-${b.dataset.tab}`; });
  if (b.dataset.tab === 'net') { unseen = 0; $('net-count').textContent = ''; }
}));

function renderChecklist() {
  const has = (k) => localStorage.getItem(storeKey(NET, k));
  const on = !$('off').classList.contains('on') && $('boot').classList.contains('done');
  const st = { on, setup: has('setup.done'), link: has('hl.acct') || has('pac.acct') };
  document.querySelectorAll('#checklist li').forEach((li) => li.classList.toggle('done', !!st[li.dataset.k]));
}

if (MODE === 'real') {
  $('net-label').textContent = NET;
  $('reset-net').textContent = NET;
  const E = W.ENDPOINTS[NETU];
  $('faucets').innerHTML = NET === 'mainnet'
    ? 'Mainnet uses your real USDC on <a href="https://app.hyperliquid.xyz" target="_blank" rel="noopener">Hyperliquid</a> or <a href="https://app.pacifica.fi" target="_blank" rel="noopener">Pacifica</a>.'
    : `<a href="https://test-app.pacifica.fi" target="_blank" rel="noopener">Pacifica testnet</a> (access code <code>Pacifica</code>, then the faucet) is the quick one. The <a href="${E.hlApp}/drip" target="_blank" rel="noopener">Hyperliquid testnet faucet</a> needs a mainnet deposit from the same address first.`;
  setInterval(renderChecklist, 1500);
  renderChecklist();

  $('relay-url').value = RELAY;
  const ping = async () => {
    try {
      const r = await fetch(`${RELAY}/health`);
      const j = await r.json();
      $('relay-status').innerHTML = `<span class="ok">● online</span> ${RELAY} · ${j.rooms} rooms · AI: ${j.llm ? 'live model' : 'preset answers'}`;
    } catch {
      $('relay-status').innerHTML = `<span class="no">● offline</span> ${RELAY}. Trading works without it; rooms, news, the AI and QR linking need it. Check the relay URL in Settings.`;
    }
  };
  ping();
  setInterval(ping, 15000);
  $('relay-save').addEventListener('click', () => { set('relay', $('relay-url').value.trim().replace(/\/$/, '')); location.reload(); });
  $('factory').addEventListener('click', () => {
    if (!confirm(`Erase this simulated device on ${NET}?`)) return;
    for (const k of Object.keys(localStorage)) if (k.startsWith(`dw.fw.${NET}.`)) localStorage.removeItem(k);
    location.reload();
  });

  const showEvm = async (addr) => {
    $('w-hl').textContent = short(addr, 8, 6);
    try { $('eq-hl').textContent = `${(await W.hlEquity(NETU, addr)).toFixed(2)} USDC`; } catch { $('eq-hl').textContent = ''; }
  };
  const showSol = async (acct) => {
    $('w-pac').textContent = short(acct, 8, 6);
    try { const v = await W.pacEquity(NETU, acct); $('eq-pac').textContent = v === null ? 'no deposit yet' : `${v.toFixed(2)} USDC`; } catch { $('eq-pac').textContent = ''; }
  };
  $('c-hl').addEventListener('click', async () => { try { const { address } = await W.connectEvm(); showEvm(address); } catch (e) { alert(e.message); } });
  $('c-pac').addEventListener('click', async () => { try { showSol(await W.connectSol()); } catch (e) { alert(e.message); } });
  const linked = (v) => localStorage.getItem(storeKey(NET, `${v}.acct`));
  if (linked('hl')) showEvm(linked('hl'));
  if (linked('pac')) showSol(linked('pac'));

  $('scan').addEventListener('click', async () => {
    const link = W.parseLink(currentQr);
    if (!link) return;
    const logEl = $('scan-log');
    logEl.hidden = false;
    logEl.textContent = '';
    const log = (t) => { logEl.textContent += t + '\n'; };
    $('scan').disabled = true;
    try {
      const acct = await W.approveLink(link, link.relay || RELAY, log);
      log(`Done. The device is linked to ${short(acct)}.`);
      if (link.venue === 'hl') showEvm(acct); else showSol(acct);
    } catch (e) {
      log(`Stopped: ${e.message || e}`);
    }
    $('scan').disabled = false;
  });
}

function renderScan() {
  if (MODE !== 'real') return;
  const q = currentQr && W.parseLink(currentQr);
  $('scan').disabled = !q;
  $('scan-detail').hidden = !q;
  if (!q) {
    $('scan-text').textContent = 'No QR on the device yet. On the device: setup step 4, or Menu → Accounts → a venue.';
    return;
  }
  const venue = q.venue === 'hl' ? 'Hyperliquid' : q.venue === 'pac' ? 'Pacifica' : 'Lighter';
  $('scan-text').innerHTML = `The device shows a link QR for <b>${venue}</b> (${q.net.toLowerCase()}). Approve it here with a browser wallet, or scan it with your phone.`;
  $('scan-detail').innerHTML = `<div class="approve"><div class="eyebrow">${venue} · approval request</div>
    <dl><dt>Device key</dt><dd>${short(q.agent, 10, 6)}</dd><dt>Can</dt><dd class="ok">place, change and cancel orders</dd>
    <dt>Cannot</dt><dd class="no">withdraw or transfer your funds</dd><dt>Wallet</dt><dd>${q.venue === 'hl' ? 'EVM (MetaMask, Rabby…)' : 'Solana (Phantom, Backpack…)'}</dd></dl></div>`;
  document.querySelector('[data-tab="wallets"]').click();
}

/* ---------------- mic and network log ---------------- */
const SAY = ['Long SOL, five x, fifty USDC', 'Short ETH three x twenty USDC on Pacifica', 'Close my SOL', 'How far is my SOL long from liquidation?', 'Hello from the simulator!'];
$('say-chips').innerHTML = SAY.map((s) => `<button type="button">${s}</button>`).join('');
$('say-chips').addEventListener('click', (e) => { if (e.target.tagName === 'BUTTON') $('say').value = e.target.textContent; });

let unseen = 0;
const HOSTS = { hyperliquid: ['hl', 'HL'], pacifica: ['pac', 'PAC'], zklighter: ['lit', 'LIT'], relay: ['relay', 'RELAY'], localhost: ['relay', 'RELAY'] };
const POLL = /\/(info|account|positions|orders|orderBooks|orderBookDetails|nextNonce|rooms|news|link\/|kline|candlesticks|history|prices|health)/;
const esc = (s) => String(s ?? '').replace(/[&<>]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;' })[c]);
const pretty = (s) => { try { return JSON.stringify(JSON.parse(s), null, 2); } catch { return s; } };
onLog((e) => {
  const isPoll = (e.kind === 'http' && !e.notes?.length && POLL.test(e.path) && !(e.req && e.req.includes('"type":"approveAgent"'))) || e.kind === 'ws-in';
  if (isPoll && !$('show-poll').checked) return;
  const key = Object.keys(HOSTS).find((k) => e.host.includes(k));
  const [cls, tag] = key ? HOSTS[key] : ['', e.host];
  const status = e.kind === 'http' ? e.status : e.kind === 'ws-out' ? 'WS ↑' : 'WS ↓';
  const err = e.kind === 'http' && (e.status >= 400 || e.status <= 0 || /"status":"err"|"success":false|"code":2\d{4}/.test(e.res || ''));
  const el = document.createElement('details');
  el.className = 'req';
  const req = e.req && e.req.startsWith('tx_type=') ? decodeURIComponent(e.req) : e.req;
  el.innerHTML = `<summary><span class="host ${cls}">${tag}</span><span class="m">${e.method || ''}</span><span class="p">${esc(e.path)}</span><span class="s ${err ? 'err' : ''}">${status}</span></summary>
    ${e.notes?.length ? `<div class="notes">${e.notes.map((n) => `<div class="${n.startsWith('✓') ? 'ok' : n.startsWith('✗') ? 'no' : ''}">${esc(n)}</div>`).join('')}</div>` : ''}
    ${req ? `<pre>→ ${esc(pretty(req))}</pre>` : ''}${e.res ? `<pre>← ${esc(pretty(e.res).slice(0, 4000))}</pre>` : ''}`;
  const list = $('net');
  list.prepend(el);
  while (list.children.length > 150) list.lastChild.remove();
  if ($('tab-net').hidden) { unseen++; $('net-count').textContent = unseen > 99 ? '99+' : String(unseen); }
});
$('net-clear').addEventListener('click', () => { $('net').innerHTML = ''; });
