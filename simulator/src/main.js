// Simulator page: boots the mocks, then the firmware (WebAssembly), and wires up the
// device controls, the phone panel and the network log.
import * as mk from './market.js';
import * as L from './ledger.js';
import { onLog } from './net.js';
import './mock_hyperliquid.js';
import './mock_pacifica.js';
import './mock_lighter.js';
import { startRelay } from './mock_relay.js';
import { approveQr, parseQr, provisionDemo, resetAll, tryWithdrawWithDeviceKey, wallets } from './phone.js';
import { createHost, IN } from './host.js';
import { short } from './util.js';

const $ = (id) => document.getElementById(id);
const canvas = $('screen');
const ctx = canvas.getContext('2d');
const image = ctx.createImageData(320, 480);
const pixels = new Uint32Array(image.data.buffer);

/* ---------------- device UI hooks ---------------- */
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
  listen(on) {
    listening = on;
    $('ptt').classList.toggle('on', on);
    if (on) {
      recogText = '';
      if ($('use-mic').checked && recog) { try { recog.start(); } catch { /* already running */ } }
      return Promise.resolve();
    }
    return new Promise((resolve) => {
      const typed = () => $('say').value.trim() || $('say').placeholder;
      if ($('use-mic').checked && recog) {
        recog.onend = () => resolve(recogText || typed());
        try { recog.stop(); } catch { resolve(typed()); }
        setTimeout(() => resolve(recogText || typed()), 2500);
      } else {
        setTimeout(() => resolve(typed()), 350);
      }
    });
  },
};

const SR = window.SpeechRecognition || window.webkitSpeechRecognition;
if (SR) {
  recog = new SR();
  recog.lang = 'en-US';
  recog.interimResults = false;
  recog.onresult = (e) => { recogText = Array.from(e.results).map((r) => r[0].transcript).join(' '); $('say').value = recogText; };
} else {
  $('use-mic').disabled = true;
  $('use-mic').parentElement.title = 'Speech recognition is not available in this browser';
}

/* ---------------- boot ---------------- */
const host = createHost(ui);

async function boot() {
  await mk.initMarket();
  setInterval(() => { mk.tick(1); L.matchResting(); }, 1000);
  startRelay();
  await provisionDemo();
  renderWallets();
  setInterval(renderWallets, 2000);
  const mod = await window.createWalkie({ dwHost: host, locateFile: (f) => new URL(`fw/${f}`, import.meta.url).href, print: () => {}, printErr: (e) => console.warn(e) });
  host.attach(mod);
  $('boot').classList.add('done');
}
boot().catch((e) => { console.error(e); $('boot').textContent = 'Could not start the firmware: ' + e.message; });

/* ---------------- controls ---------------- */
const send = (code) => host.input(code);
document.querySelectorAll('[data-in]').forEach((b) => b.addEventListener('click', () => send(+b.dataset.in)));
$('k-trade').addEventListener('click', () => send(IN.TRADE));
$('k-power').addEventListener('click', () => send(IN.POWER));
$('k-wheel').addEventListener('click', () => send(IN.PRESS));
let wheelAcc = 0;
$('k-wheel').addEventListener('wheel', (e) => {
  e.preventDefault();
  wheelAcc += e.deltaY;
  while (Math.abs(wheelAcc) >= 60) { send(wheelAcc > 0 ? IN.DOWN : IN.UP); wheelAcc -= Math.sign(wheelAcc) * 60; }
}, { passive: false });

const ptt = $('ptt');
const pttDown = () => { if (!listening) send(IN.PTT_DOWN); };
const pttUp = () => { if (listening) send(IN.PTT_UP); };
ptt.addEventListener('pointerdown', (e) => { ptt.setPointerCapture(e.pointerId); pttDown(); });
ptt.addEventListener('pointerup', pttUp);
ptt.addEventListener('pointercancel', pttUp);

window.addEventListener('keydown', (e) => {
  if (e.target instanceof HTMLInputElement) return;
  const k = e.key;
  if (k === 'ArrowUp') send(IN.UP);
  else if (k === 'ArrowDown') send(IN.DOWN);
  else if (k === 'Escape' || k === 'Backspace') send(IN.PRESS);
  else if (k === 'Enter' || k === ' ') { send(IN.TRADE); document.querySelector('.tradekey').classList.add('down'); }
  else if (k === 'p' || k === 'P') send(IN.POWER);
  else if ((k === 'm' || k === 'M') && !e.repeat) pttDown();
  else return;
  e.preventDefault();
});
window.addEventListener('keyup', (e) => {
  if (e.key === 'm' || e.key === 'M') pttUp();
  if (e.key === 'Enter' || e.key === ' ') document.querySelector('.tradekey').classList.remove('down');
});

function toDevice(e) {
  const r = canvas.getBoundingClientRect();
  return [Math.round(((e.clientX - r.left) / r.width) * 320), Math.round(((e.clientY - r.top) / r.height) * 480)];
}
let touching = false;
canvas.addEventListener('pointerdown', (e) => { touching = true; canvas.setPointerCapture(e.pointerId); host.touch(...toDevice(e), true); });
canvas.addEventListener('pointermove', (e) => { if (touching) host.touch(...toDevice(e), true); });
const lift = (e) => { if (touching) { touching = false; host.touch(...toDevice(e), false); } };
canvas.addEventListener('pointerup', lift);
canvas.addEventListener('pointercancel', lift);
canvas.addEventListener('wheel', (e) => {
  e.preventDefault();
  wheelAcc += e.deltaY;
  while (Math.abs(wheelAcc) >= 60) { send(wheelAcc > 0 ? IN.DOWN : IN.UP); wheelAcc -= Math.sign(wheelAcc) * 60; }
}, { passive: false });

/* ---------------- tabs ---------------- */
document.querySelectorAll('.tabs button').forEach((b) => b.addEventListener('click', () => {
  document.querySelectorAll('.tabs button').forEach((x) => x.classList.toggle('on', x === b));
  document.querySelectorAll('.tab').forEach((t) => { t.hidden = t.id !== `tab-${b.dataset.tab}`; });
  if (b.dataset.tab === 'net') { unseen = 0; $('net-count').textContent = ''; }
}));

/* ---------------- phone panel ---------------- */
const VN = { hl: 'Hyperliquid', pac: 'Pacifica', lit: 'Lighter' };
function renderWallets() {
  const rows = [
    ['hl', wallets.hl.address, wallets.hl.address.toLowerCase(), 'EVM wallet'],
    ['pac', wallets.pac.pubkey, wallets.pac.pubkey, 'Solana wallet'],
    ['lit', `account #${wallets.lit.index}`, String(wallets.lit.index), 'L1 ' + short(wallets.lit.l1)],
  ];
  $('wallets').innerHTML = rows.map(([v, label, id, kind]) => {
    const acct = L.db[v].accounts[id];
    const eq = acct ? L.summary(v, acct).equity : 0;
    const linked = localStorage.getItem(`dw.fw.${v}.acct`);
    return `<div class="wallet"><b>${VN[v]}</b><code title="${label}">${short(label, 8, 6)}</code><span class="eq">${acct ? eq.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 }) + ' USDC' : '—'}</span>
      <span class="st ${linked ? 'ok' : ''}">${kind} · ${linked ? 'device linked (trade-only)' : 'device not linked'}</span></div>`;
  }).join('');
  const b = host.battery();
  $('batt').textContent = `Battery ${Math.round(b.battery)}%${b.charging ? ' · charging' : ''}`;
}

function renderScan() {
  const q = currentQr && parseQr(currentQr);
  $('scan').disabled = !q;
  $('scan-detail').hidden = true;
  $('scan').textContent = 'Scan & review';
  $('scan-text').textContent = q
    ? `The device is showing a link QR for ${VN[q.venue]}.`
    : 'No QR on the device screen. On the device: Menu → Accounts → pick a venue that isn\'t linked (Lighter, at first).';
}

$('scan').addEventListener('click', async () => {
  const q = parseQr(currentQr);
  if (!q) return;
  const d = $('scan-detail');
  if (d.hidden) {
    const who = q.venue === 'lit' ? `API key ${short(q.pubkey, 8, 6)}` : `${short(q.agent, 8, 6)} "walkie"`;
    const signer = q.venue === 'hl' ? `${short(wallets.hl.address)} (EIP-712)` : q.venue === 'pac' ? `${short(wallets.pac.pubkey)} (Ed25519)` : `L1 ${short(wallets.lit.l1)}`;
    d.innerHTML = `<div class="approve"><div class="eyebrow">${VN[q.venue]} · approval request</div>
      <dl><dt>Device key</dt><dd>${who}</dd><dt>Can</dt><dd class="ok">place, change and cancel orders</dd>
      <dt>Cannot</dt><dd class="no">${q.venue === 'lit' ? 'withdraw anywhere but your own address' : 'withdraw or transfer funds'}</dd>
      <dt>Signed by</dt><dd>${signer}</dd></dl></div>`;
    d.hidden = false;
    $('scan').textContent = 'Approve in wallet';
    return;
  }
  $('scan').disabled = true;
  $('scan').textContent = 'Signing…';
  const r = await approveQr(currentQr);
  $('scan-text').textContent = r.ok ? `Approved. ${VN[r.venue]} is now linked on the device.` : r.msg;
  d.hidden = true;
  $('scan').textContent = 'Scan & review';
  renderWallets();
});

$('attack').addEventListener('click', async () => {
  const out = $('attack-out');
  out.hidden = false;
  out.textContent = 'Signing withdraw3 with the device key…';
  const r = await tryWithdrawWithDeviceKey();
  out.textContent = r.text.startsWith('{')
    ? `Hyperliquid answered:\n${r.text}\n\nAn agent key can sign orders, but withdrawals must be signed by the account owner. Your funds stay put.`
    : r.text;
});

const SAY = ['Long SOL, five x, five hundred USDC', 'Short ETH three x two hundred USDC on Pacifica', 'Close my HYPE', 'How far is my SOL long from liquidation?', 'Rolling into HYPE after this.'];
$('say-chips').innerHTML = SAY.map((s) => `<button type="button">${s}</button>`).join('');
$('say-chips').addEventListener('click', (e) => { if (e.target.tagName === 'BUTTON') $('say').value = e.target.textContent; });

$('usb').addEventListener('change', (e) => { host.setCharging(e.target.checked); renderWallets(); });
$('reset').addEventListener('click', () => {
  if (!confirm('Erase the device keys, your mock wallets and the simulated book?')) return;
  resetAll();
  location.reload();
});

/* ---------------- network log ---------------- */
let unseen = 0;
const HOSTS = { 'api.hyperliquid.xyz': ['hl', 'HL'], 'api.pacifica.fi': ['pac', 'PAC'], 'ws.pacifica.fi': ['pac', 'PAC'], 'mainnet.zklighter.elliot.ai': ['lit', 'LIT'], 'relay.degenwalkie.trade': ['relay', 'RELAY'] };
const POLL = /\/(info|account|positions|orders|orderBooks|orderBookDetails|nextNonce|rooms|news|link\/|kline|candlesticks|history|prices)/;
const esc = (s) => String(s ?? '').replace(/[&<>]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;' })[c]);
const pretty = (s) => { try { return JSON.stringify(JSON.parse(s), null, 2); } catch { return s; } };

onLog((e) => {
  const isPoll = (e.kind === 'http' && !e.notes?.length && POLL.test(e.path) && !(e.req && e.req.includes('"type":"approveAgent"'))) || e.kind === 'ws-in';
  if (isPoll && !$('show-poll').checked) return;
  const [cls, tag] = HOSTS[e.host] || ['', e.host];
  const status = e.kind === 'http' ? e.status : e.kind === 'ws-out' ? 'WS ↑' : 'WS ↓';
  const err = e.kind === 'http' && (e.status >= 400 || e.status <= 0 || /"status":"err"|"success":false|"error":"[^"]|"code":2\d{4}/.test(e.res || ''));
  const el = document.createElement('details');
  el.className = 'req';
  const req = e.req && e.req.startsWith('tx_type=') ? decodeURIComponent(e.req) : e.req;
  el.innerHTML = `<summary><span class="host ${cls}">${tag}</span><span class="m">${e.method || ''}</span><span class="p">${esc(e.path)}</span><span class="s ${err ? 'err' : ''}">${status}</span></summary>
    ${e.notes?.length ? `<div class="notes">${e.notes.map((n) => `<div class="${n.startsWith('✓') ? 'ok' : n.startsWith('✗') ? 'no' : ''}">${esc(n)}</div>`).join('')}</div>` : ''}
    ${req ? `<pre>→ ${esc(pretty(req))}</pre>` : ''}${e.res ? `<pre>← ${esc(pretty(e.res).slice(0, 4000))}</pre>` : ''}`;
  if (e.notes?.length) el.open = false;
  const list = $('net');
  list.prepend(el);
  while (list.children.length > 150) list.lastChild.remove();
  if ($('tab-net').hidden) { unseen++; $('net-count').textContent = unseen > 99 ? '99+' : String(unseen); }
});
$('net-clear').addEventListener('click', () => { $('net').innerHTML = ''; });

