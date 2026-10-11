/* Shared three.js rig for the Degen Walkie Genesis enclosure.
   CAD frame: X width, Y height (up), Z thickness; front glass z = 0, back face z = -10.
   Used by DeviceViewer (OrbitControls) and ClipScroll (scroll-driven). Client-side only. */

import * as THREE from 'three';
import { STLLoader } from 'three/examples/jsm/loaders/STLLoader.js';
import { RoomEnvironment } from 'three/examples/jsm/environments/RoomEnvironment.js';

export const W = 64.5;
export const H = 112.9;  // v7: top edge raised 1.4 mm for the ESP32 antenna (y -55.75 .. 57.15)
export const Y_TOP = 57.15, Y_MID = 0.7;
export const WALL = 1.6;
export const XI = W / 2 - WALL; // 30.65 — inner face of the side walls (keycap local z = 0)
export const Z_KEY = -5.0; // keys are centred on the edge (half of the 10 mm thickness)
export const NEON = 0x14f195;
export const HOT = 0x9945ff;

type Vec3 = [number, number, number];
type Basis = [Vec3, Vec3, Vec3];

// basis: world directions of a part's local X, Y, Z axes (columns); must be right-handed.
// Bump when any STL in public/models changes: the files are served immutable, the query string busts the cache.
const MODEL_VERSION = '8.3';
const RIGHT_WALL: Basis = [[0, 1, 0], [0, 0, 1], [1, 0, 0]];
const LEFT_WALL: Basis = [[0, 1, 0], [0, 0, -1], [-1, 0, 0]];
const TOP_WALL: Basis = [[1, 0, 0], [0, 0, -1], [0, 1, 0]];

export type PartMat = 'shell' | 'back' | 'wheel' | 'key' | 'trade' | 'light' | 'clip';
export interface Part { file: string; mat: PartMat; edges?: boolean; pos?: Vec3; basis?: Basis; clip?: boolean }

export const PARTS: Part[] = [
  { file: 'front_shell', mat: 'shell', edges: true },
  { file: 'back_cover', mat: 'back', edges: true },
  { file: 'wheel', mat: 'wheel' },
  { file: 'key_power', mat: 'key', pos: [-XI, 30, Z_KEY], basis: LEFT_WALL },
  { file: 'key_trade', mat: 'trade', pos: [XI, 30, Z_KEY], basis: RIGHT_WALL },
  { file: 'antenna_light', mat: 'light', pos: [-10.75, 57.15, -5.5], basis: TOP_WALL },
  { file: 'clip_4g', mat: 'clip', clip: true, edges: true },
];

export const MODEL_FILES = Array.from(new Set(PARTS.map((p) => p.file)));

/* Camera presets: azimuth (rad, from +Z around Y), elevation (rad), zoom factor */
export const VIEWS: Record<string, { az: number; el: number; zoom: number }> = {
  front: { az: 0.34, el: 0.1, zoom: 1.0 },
  back: { az: Math.PI - 0.3, el: 0.1, zoom: 1.0 },
  right: { az: Math.PI / 2 - 0.38, el: 0.12, zoom: 1.08 },
  left: { az: -Math.PI / 2 + 0.38, el: 0.12, zoom: 1.08 },
  top: { az: 0.18, el: 1.22, zoom: 1.75 },
};

export function viewDir(name: string): THREE.Vector3 {
  const v = VIEWS[name] || VIEWS.front;
  return new THREE.Vector3(Math.sin(v.az) * Math.cos(v.el), Math.sin(v.el), Math.cos(v.az) * Math.cos(v.el)).normalize();
}

/* Feature callouts (device coordinates, mm) for the hardware page */
export interface Feature { id: string; pos: Vec3; view: string; label: string; title: string; text: string; where: string }
export const FEATURES: Feature[] = [
  { id: 'wheel', pos: [33.9, 45.5, -5.35], view: 'right', label: 'Thumb wheel', title: 'Thumb wheel', text: 'Knurled, set into the shell. Roll: move the focus, adjust a value, flip pages. Press: back (or menu at the top level).', where: 'Top-right edge' },
  { id: 'trade', pos: [32.6, 30, -5.0], view: 'right', label: 'Trade key', title: 'Trade key', text: 'Open / enter. Opens the focused item; on a confirmation sheet it signs. 1.3 mm proud so you find it blind.', where: 'Right edge' },
  { id: 'power', pos: [-32.6, 30, -5.0], view: 'left', label: 'Power key', title: 'Power key', text: 'Short press: screen off / on, still listening. Hold 2 s: slide-to-power-off sheet.', where: 'Left edge' },
  { id: 'light', pos: [-10.75, 63, -5.5], view: 'top', label: 'Status light', title: 'Status light', text: 'RGB nub on the top edge. Blue, green, red, yellow, purple — legend below.', where: 'Top edge' },
  { id: 'usb', pos: [32.3, -30, -5.8], view: 'right', label: 'USB-C', title: 'USB-C', text: 'Charging and signed firmware updates. No debug port.', where: 'Right edge, low' },
  { id: 'camera', pos: [-3.25, 49.1, -10.2], view: 'back', label: 'Camera (QR)', title: 'Camera', text: 'Rear camera for QR: Wi-Fi setup and account linking.', where: 'Back' },
  { id: 'pogo', pos: [20.25, -20, -10.2], view: 'back', label: '6 gold contacts', title: '6 gold contacts', text: 'Pogo pads for the magnetic 4G back-clip.', where: 'Back' },
  { id: 'speaker', pos: [17, -45.75, -10.2], view: 'back', label: 'Speaker grille', title: 'Speaker grille', text: 'Twenty ports, bottom-right of the back cover.', where: 'Back' },
  { id: 'mic', pos: [12.8, 51.5, -10.2], view: 'back', label: 'Mic', title: 'Mic', text: 'Single port for the MEMS microphone, top of the back cover, far from the speaker.', where: 'Back' },
];

// Pogo contact row (from the CAD): x = 13.4 + 2.54·i, y = -20, Ø2.6 holes through the 1.4 mm back cover
const POGO = { x0: 12.75, pitch: 3.0, y: -20, n: 6 };
const CAM = { x: -3.25, y: 49.1, d: 7.0 };

/* ---------------- Screen texture: Room view, drawn in 320×480 device px ---------------- */

const SCREEN_W = 60.5, SCREEN_H = 94.0, SCREEN_CY = -3.5, PX = 10; // canvas px per mm
const ACT_W = 55.7, ACT_H = 83.5, ACT_CY = -0.5;

function roundRect(ctx: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, r: number) {
  ctx.beginPath();
  ctx.moveTo(x + r, y);
  ctx.arcTo(x + w, y, x + w, y + h, r);
  ctx.arcTo(x + w, y + h, x, y + h, r);
  ctx.arcTo(x, y + h, x, y, r);
  ctx.arcTo(x, y, x + w, y, r);
  ctx.closePath();
}

function fontStack(kind: 'display' | 'sans' | 'mono'): string {
  if (typeof document === 'undefined') return 'sans-serif';
  const v = getComputedStyle(document.documentElement).getPropertyValue(kind === 'display' ? '--font-archivo' : kind === 'mono' ? '--font-mono' : '--font-inter').trim();
  const fb = kind === 'display' ? 'Archivo, Inter, sans-serif' : kind === 'mono' ? '"JetBrains Mono", ui-monospace, Menlo, Consolas, monospace' : 'Inter, system-ui, sans-serif';
  return v ? `${v}, ${fb}` : fb;
}

function displayFont(ctx: CanvasRenderingContext2D, size: number, weight = 900) {
  ctx.font = `${weight} ${size}px ${fontStack('display')}`;
  try { (ctx as unknown as { fontStretch: string }).fontStretch = 'extra-condensed'; } catch { /* older browsers */ }
}
function textFont(ctx: CanvasRenderingContext2D, size: number, weight = 400, mono = false) {
  ctx.font = `${weight} ${size}px ${fontStack(mono ? 'mono' : 'sans')}`;
  try { (ctx as unknown as { fontStretch: string }).fontStretch = 'normal'; } catch { /* ignore */ }
}

export function makeScreenCanvas(): HTMLCanvasElement {
  const c = document.createElement('canvas');
  c.width = Math.round(SCREEN_W * PX);
  c.height = Math.round(SCREEN_H * PX);
  return c;
}

/* Screen carousel: voice order → chart → positions → news → live chat. Each ~4.5 s with a short crossfade.
   Mirrors the screens in components/screens; drawn in 320×480 device px. */
const SCENE_DUR = 4.5, FADE = 0.45;
const C = { INK: '#e9ecf0', MUTE: '#8a929e', DIM: '#6c7480', LINE: '#1f242b', NEON: '#14f195', HOT: '#c89bff', BAD: '#ff4d4d', WARN: '#ffb020', BG: '#161a20' };

type Scene = { label: string; sub: string; hint: [string, string]; body: (ctx: CanvasRenderingContext2D, t: number) => void };

function statusBar(ctx: CanvasRenderingContext2D, label: string, sub: string, idx: number, n: number) {
  textFont(ctx, 10.5, 600, true);
  ctx.fillStyle = C.NEON; ctx.fillText(label, 14, 11);
  ctx.fillStyle = C.MUTE; ctx.fillText(sub, 14 + ctx.measureText(label).width + 6, 11);
  ctx.strokeStyle = C.MUTE; ctx.lineWidth = 1.6; ctx.lineCap = 'round';
  for (let i = 1; i <= 3; i++) { ctx.beginPath(); ctx.arc(255, 23, i * 3.2, Math.PI * 1.25, Math.PI * 1.75); ctx.stroke(); }
  ctx.fillStyle = C.MUTE; ctx.beginPath(); ctx.arc(255, 22, 1.3, 0, Math.PI * 2); ctx.fill();
  ctx.strokeStyle = C.MUTE; ctx.lineWidth = 1.2; roundRect(ctx, 268, 12, 20, 10, 2); ctx.stroke();
  ctx.fillRect(289, 15, 2, 4);
  ctx.fillStyle = C.NEON; ctx.fillRect(270, 14, 14, 6);
  ctx.fillStyle = C.MUTE; textFont(ctx, 9.5, 500, true); ctx.fillText('86', 295, 12);
  // page dots
  for (let i = 0; i < n; i++) { ctx.beginPath(); ctx.arc(196 + i * 9, 17, i === idx ? 2.6 : 1.8, 0, Math.PI * 2); ctx.fillStyle = i === idx ? C.NEON : '#3a414b'; ctx.fill(); }
  ctx.strokeStyle = '#1a1e24'; ctx.lineWidth = 1; ctx.beginPath(); ctx.moveTo(0, 32); ctx.lineTo(320, 32); ctx.stroke();
}

function hintBar(ctx: CanvasRenderingContext2D, roll: string, trade: string) {
  ctx.strokeStyle = '#1a1e24'; ctx.lineWidth = 1; ctx.beginPath(); ctx.moveTo(0, 446); ctx.lineTo(320, 446); ctx.stroke();
  ctx.beginPath(); ctx.moveTo(107, 446); ctx.lineTo(107, 480); ctx.moveTo(213, 446); ctx.lineTo(213, 480); ctx.stroke();
  const slots: [string, string, string][] = [['⟲ ROLL', roll, '#c5ccd6'], ['● PRESS', 'BACK', '#c5ccd6'], ['▮ TRADE', trade, C.NEON]];
  ctx.textAlign = 'center';
  slots.forEach(([a, b, col], i) => {
    const sx = 53 + i * 106.5;
    textFont(ctx, 7, 500, true); ctx.fillStyle = '#5c6470'; ctx.fillText(a, sx, 453);
    textFont(ctx, 8.5, 600, true); ctx.fillStyle = col; ctx.fillText(b, sx, 465);
  });
  ctx.textAlign = 'left';
}

function eyebrow(ctx: CanvasRenderingContext2D, s: string, x: number, y: number) { textFont(ctx, 8.5, 600, true); ctx.fillStyle = C.MUTE; ctx.fillText(s, x, y); }

function micButton(ctx: CanvasRenderingContext2D, t: number, cx: number, cy: number, r: number, sub: string) {
  const pulse = 0.5 + 0.5 * Math.sin(t * 2.2);
  ctx.beginPath(); ctx.arc(cx, cy, r + 7 + pulse * 5, 0, Math.PI * 2);
  ctx.fillStyle = `rgba(20, 241, 149, ${0.07 + pulse * 0.07})`; ctx.fill();
  ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI * 2);
  ctx.lineWidth = 3; ctx.strokeStyle = C.NEON; ctx.stroke();
  const bars = 9, bw = 3, gap = 3, total = bars * bw + (bars - 1) * gap;
  for (let i = 0; i < bars; i++) {
    const hgt = 5 + 16 * Math.abs(Math.sin(t * 3 + i * 0.9));
    ctx.fillStyle = C.NEON;
    ctx.fillRect(cx - total / 2 + i * (bw + gap), cy - 8 - hgt / 2, bw, hgt);
  }
  displayFont(ctx, 12, 900);
  ctx.fillStyle = C.NEON; ctx.textAlign = 'center'; ctx.fillText('HOLD', cx, cy + 12);
  textFont(ctx, 7.5, 500, true); ctx.fillStyle = C.MUTE; ctx.fillText(sub, cx, cy + 26);
  ctx.textAlign = 'left';
}

/* deterministic candles */
const CANDLES = (() => {
  let s = 7, p = 205;
  const rnd = () => { s = (s * 16807) % 2147483647; return s / 2147483647; };
  const out: [number, number, number, number][] = [];
  for (let i = 0; i < 34; i++) {
    const o = p; const c = o + (rnd() - 0.42) * 2.6; const hi = Math.max(o, c) + rnd() * 1.4; const lo = Math.min(o, c) - rnd() * 1.4;
    out.push([o, hi, lo, c]); p = c;
  }
  return out;
})();

function voiceScene(ctx: CanvasRenderingContext2D, t: number) {
  eyebrow(ctx, 'YOU SAID', 14, 42);
  roundRect(ctx, 10, 56, 300, 52, 6); ctx.fillStyle = 'rgba(255,255,255,0.03)'; ctx.fill(); ctx.strokeStyle = C.LINE; ctx.lineWidth = 1; ctx.stroke();
  textFont(ctx, 15, 500, false); ctx.fillStyle = C.INK;
  ctx.fillText('“Long SOL, five x,', 20, 64); ctx.fillText('five hundred USDC.”', 20, 84);
  eyebrow(ctx, 'TICKET', 14, 120);
  roundRect(ctx, 10, 134, 300, 132, 6);
  ctx.fillStyle = 'rgba(20,241,149,0.06)'; ctx.fill(); ctx.lineWidth = 2; ctx.strokeStyle = C.NEON; ctx.stroke();
  const rows: [string, string, string][] = [['SIDE', 'LONG', C.NEON], ['MARKET', 'SOL-PERP · Hyperliquid', C.INK], ['LEVERAGE', '5×', C.INK], ['SIZE', '500.00 USDC', C.INK], ['LIQ. PRICE', '171.20', C.BAD]];
  rows.forEach(([k, v, col], i) => {
    const ry = 146 + i * 23;
    textFont(ctx, 9, 500, true); ctx.fillStyle = C.MUTE; ctx.textAlign = 'left'; ctx.fillText(k, 22, ry + 2);
    textFont(ctx, i === 0 ? 14 : 12.5, i === 0 ? 800 : 600, true); ctx.fillStyle = col; ctx.textAlign = 'right'; ctx.fillText(v, 298, ry);
  });
  ctx.textAlign = 'left';
  roundRect(ctx, 10, 278, 300, 34, 6); ctx.fillStyle = 'rgba(20,241,149,0.12)'; ctx.fill();
  ctx.fillStyle = C.NEON; ctx.fillRect(22, 287, 6, 16);
  textFont(ctx, 10.5, 700, true); ctx.fillText('PRESS THE TRADE KEY TO SEND', 38, 290);
  micButton(ctx, t, 160, 384, 38, 'TO SPEAK');
}

function chartScene(ctx: CanvasRenderingContext2D, t: number) {
  const live = CANDLES[CANDLES.length - 1][3] + Math.sin(t * 1.7) * 0.6 + Math.sin(t * 4.1) * 0.25;
  displayFont(ctx, 22, 900); ctx.fillStyle = C.INK; ctx.fillText('SOL-PERP', 14, 42);
  textFont(ctx, 8.5, 500, true); ctx.fillStyle = C.MUTE; ctx.fillText('HYPERLIQUID · 15M', 14, 66);
  textFont(ctx, 17, 700, true); ctx.fillStyle = C.NEON; ctx.textAlign = 'right'; ctx.fillText(live.toFixed(2), 306, 42);
  textFont(ctx, 9, 500, true); ctx.fillText('+3.1% · 24H', 306, 64); ctx.textAlign = 'left';
  // plot
  const x0 = 14, x1 = 280, y0 = 84, y1 = 300;
  const all = CANDLES.flatMap((c) => [c[1], c[2]]).concat([live, 199.2]);
  const hi = Math.max(...all) + 1, lo = Math.min(...all) - 1;
  const Y = (v: number) => y0 + ((hi - v) / (hi - lo)) * (y1 - y0);
  ctx.strokeStyle = '#1d2229'; ctx.lineWidth = 1;
  for (let i = 0; i <= 4; i++) { const yy = y0 + (i * (y1 - y0)) / 4; ctx.beginPath(); ctx.moveTo(x0, yy); ctx.lineTo(x1, yy); ctx.stroke(); textFont(ctx, 7.5, 500, true); ctx.fillStyle = '#56606c'; ctx.fillText((hi - (i * (hi - lo)) / 4).toFixed(1), x1 + 4, yy - 4); }
  const n = CANDLES.length, step = (x1 - x0) / n;
  CANDLES.forEach((c, i) => {
    const [o, h, l] = c; const cl = i === n - 1 ? live : c[3];
    const up = cl >= o, col = up ? C.NEON : C.BAD, cx = x0 + step * (i + 0.5);
    ctx.strokeStyle = col; ctx.lineWidth = 1; ctx.beginPath(); ctx.moveTo(cx, Y(Math.max(h, cl))); ctx.lineTo(cx, Y(Math.min(l, cl))); ctx.stroke();
    ctx.fillStyle = col; const top = Y(Math.max(o, cl)), bot = Y(Math.min(o, cl)); ctx.fillRect(cx - step * 0.32, top, step * 0.64, Math.max(1.2, bot - top));
  });
  // entry line
  const entry = 207.6, ey = Y(entry);
  ctx.setLineDash([4, 3]); ctx.strokeStyle = C.HOT; ctx.beginPath(); ctx.moveTo(x0, ey); ctx.lineTo(x1, ey); ctx.stroke(); ctx.setLineDash([]);
  textFont(ctx, 8, 600, true); ctx.fillStyle = C.HOT; ctx.fillText('ENTRY 207.60', x0 + 2, ey - 11);
  // position card
  roundRect(ctx, 10, 314, 300, 74, 6); ctx.fillStyle = 'rgba(20,241,149,0.06)'; ctx.fill(); ctx.lineWidth = 2; ctx.strokeStyle = C.NEON; ctx.stroke();
  textFont(ctx, 9, 600, true); ctx.fillStyle = C.NEON; ctx.fillText('LONG 5× · 500 USDC', 22, 324);
  textFont(ctx, 20, 700, true); const pnl = (live - entry) / entry * 2500; ctx.fillStyle = pnl >= 0 ? C.NEON : C.BAD; ctx.fillText(`${pnl >= 0 ? '+' : ''}${pnl.toFixed(2)}`, 22, 342);
  textFont(ctx, 9, 500, true); ctx.fillStyle = C.MUTE; ctx.textAlign = 'right'; ctx.fillText('LIQ 171.20 · 19% AWAY', 298, 326); ctx.fillText('FUNDING +0.01%/H', 298, 344); ctx.fillText('USDC', 298, 362); ctx.textAlign = 'left';
  // quick actions
  const btns: [string, string][] = [['ADD', C.NEON], ['HALF', C.INK], ['CLOSE', C.BAD]];
  btns.forEach(([b, col], i) => { const bx = 10 + i * 102; roundRect(ctx, bx, 398, 96, 32, 5); ctx.strokeStyle = i === 0 ? C.NEON : '#2c323b'; ctx.lineWidth = 1.4; ctx.stroke(); textFont(ctx, 10, 700, true); ctx.fillStyle = col; ctx.textAlign = 'center'; ctx.fillText(b, bx + 48, 408); ctx.textAlign = 'left'; });
}

function positionsScene(ctx: CanvasRenderingContext2D, t: number) {
  eyebrow(ctx, 'EQUITY · 3 VENUES', 14, 42);
  displayFont(ctx, 30, 900); ctx.fillStyle = C.INK; ctx.fillText('6,184.20', 14, 56);
  textFont(ctx, 10, 600, true); ctx.fillStyle = C.NEON; ctx.fillText('+212.48 TODAY', 16, 92);
  const pos: [string, string, string, number, number, string][] = [
    ['SOL-PERP', 'LONG 5× · HYPERLIQUID', '+118.40', 0.81, 0, C.NEON],
    ['BTC-PERP', 'SHORT 3× · LIGHTER', '+64.02', 0.62, 1, C.NEON],
    ['ETH-PERP', 'LONG 10× · PACIFICA', '-31.75', 0.27, 2, C.BAD],
    ['HYPE-PERP', 'LONG 2× · HYPERLIQUID', '+61.81', 0.9, 3, C.NEON],
  ];
  pos.forEach(([m, sub, p, safe, i, col]) => {
    const y = 112 + i * 78;
    const focus = i === Math.floor(t / 1.1) % 4;
    roundRect(ctx, 10, y, 300, 68, 6); ctx.fillStyle = focus ? 'rgba(20,241,149,0.06)' : 'rgba(255,255,255,0.025)'; ctx.fill();
    ctx.lineWidth = focus ? 2 : 1; ctx.strokeStyle = focus ? C.NEON : C.LINE; ctx.stroke();
    textFont(ctx, 13, 700, true); ctx.fillStyle = C.INK; ctx.fillText(m, 22, y + 10);
    textFont(ctx, 8.5, 500, true); ctx.fillStyle = C.MUTE; ctx.fillText(sub, 22, y + 30);
    textFont(ctx, 14, 700, true); ctx.fillStyle = col; ctx.textAlign = 'right'; ctx.fillText(p, 298, y + 10); ctx.textAlign = 'left';
    // distance to liquidation bar
    ctx.fillStyle = '#232931'; ctx.fillRect(22, y + 48, 276, 6);
    ctx.fillStyle = safe < 0.35 ? C.WARN : C.NEON; ctx.fillRect(22, y + 48, 276 * safe, 6);
    textFont(ctx, 7.5, 500, true); ctx.fillStyle = safe < 0.35 ? C.WARN : '#56606c'; ctx.textAlign = 'right'; ctx.fillText(safe < 0.35 ? 'NEAR LIQ.' : 'LIQ. DISTANCE', 298, y + 36); ctx.textAlign = 'left';
  });
}

function newsScene(ctx: CanvasRenderingContext2D, t: number) {
  eyebrow(ctx, 'TRADING NEWS · YOUR MARKETS', 14, 42);
  const news: [string, string, string, string][] = [
    ['2m', 'SOL', 'Funding flips negative on SOL-PERP across three venues', C.NEON],
    ['9m', 'BTC', 'Spot ETF inflows top $410M for the session', C.NEON],
    ['14m', 'ETH', '$38M of longs liquidated in 10 minutes', C.BAD],
    ['31m', 'HYPE', 'Open interest hits a new high', C.WARN],
    ['1h', 'MACRO', 'CPI in 45 minutes — volatility expected', C.HOT],
  ];
  news.forEach(([ago, tag, line, col], i) => {
    const y = 60 + i * 74;
    const fresh = i === 0 && Math.sin(t * 4) > 0;
    roundRect(ctx, 10, y, 300, 64, 6); ctx.fillStyle = i === 0 ? 'rgba(20,241,149,0.05)' : 'rgba(255,255,255,0.025)'; ctx.fill(); ctx.strokeStyle = i === 0 ? C.NEON : C.LINE; ctx.lineWidth = i === 0 ? 2 : 1; ctx.stroke();
    textFont(ctx, 8.5, 700, true);
    roundRect(ctx, 20, y + 10, ctx.measureText(tag).width + 12, 16, 3); ctx.fillStyle = col; ctx.globalAlpha *= 0.18; ctx.fill(); ctx.globalAlpha /= 0.18;
    textFont(ctx, 8.5, 700, true); ctx.fillStyle = col; ctx.fillText(tag, 26, y + 13);
    textFont(ctx, 8, 500, true); ctx.fillStyle = fresh ? C.NEON : C.MUTE; ctx.textAlign = 'right'; ctx.fillText(i === 0 ? 'NEW · ' + ago : ago, 298, y + 13); ctx.textAlign = 'left';
    textFont(ctx, 12, 500, false); ctx.fillStyle = C.INK;
    const words = line.split(' '); let l1 = '', l2 = '';
    for (const w of words) { if (ctx.measureText(l1 + ' ' + w).width < 270 && !l2) l1 = l1 ? l1 + ' ' + w : w; else l2 = l2 ? l2 + ' ' + w : w; }
    ctx.fillText(l1, 20, y + 32); if (l2) ctx.fillText(l2, 20, y + 47);
  });
}

function chatScene(ctx: CanvasRenderingContext2D, t: number) {
  roundRect(ctx, 10, 38, 300, 46, 6); ctx.fillStyle = 'rgba(20,241,149,0.06)'; ctx.fill(); ctx.lineWidth = 2; ctx.strokeStyle = C.NEON; ctx.stroke();
  displayFont(ctx, 22, 900); ctx.fillStyle = C.INK; ctx.fillText('ROOM #SOL', 18, 45);
  textFont(ctx, 8.5, 500, true); ctx.fillStyle = C.MUTE; ctx.fillText('1,204 LISTENING · HEARD IN EN', 18, 69);
  textFont(ctx, 12, 600, true); ctx.fillStyle = C.NEON; ctx.textAlign = 'right'; ctx.fillText('212.40', 300, 46); textFont(ctx, 9, 500, true); ctx.fillText('+3.1%', 300, 63); ctx.textAlign = 'left';
  const msgs: [string, string, string, string, string][] = [
    ['@kaito', 'JA → EN', 'Funding just flipped on SOL.', 'Anyone adding here?', C.NEON],
    ['@mara', 'ES → EN', 'Same on Pacifica. Watching', '208 as support.', C.NEON],
    ['@lin', 'ZH → EN', 'Took profit on half,', 'stop at entry.', C.NEON],
    ['@you', 'EN', 'Long 5× from 207.6, holding.', '', C.HOT],
  ];
  const shown = 2 + Math.min(2, Math.floor((t % SCENE_DUR) / 1.3));
  let y = 98;
  msgs.slice(0, shown).forEach(([who, lang, l1, l2, col]) => {
    textFont(ctx, 9.5, 600, true); ctx.fillStyle = col; ctx.fillText(who, 14, y);
    ctx.fillStyle = C.MUTE; ctx.fillText(lang, 14 + ctx.measureText(who).width + 7, y);
    textFont(ctx, 13, 400, false); ctx.fillStyle = C.INK; ctx.fillText(l1, 14, y + 14);
    if (l2) { ctx.fillText(l2, 14, y + 30); y += 54; } else { y += 40; }
  });
  micButton(ctx, t, 160, 384, 38, 'TO TALK');
}

const SCENES: Scene[] = [
  { label: '#SOL', sub: '· LIVE ROOM', hint: ['PAGE', 'OPEN SOL'], body: chatScene },
  { label: 'SOL-PERP', sub: '· CHART', hint: ['TIMEFRAME', 'TRADE'], body: chartScene },
  { label: 'POSITIONS', sub: '· 4 OPEN', hint: ['MOVE', 'OPEN'], body: positionsScene },
  { label: 'NEWS', sub: '· LIVE', hint: ['SCROLL', 'OPEN'], body: newsScene },
  { label: '#SOL', sub: '· VOICE SHORTCUT', hint: ['EDIT', 'SEND'], body: voiceScene },
];

function drawScene(ctx: CanvasRenderingContext2D, i: number, t: number) {
  const s = SCENES[i];
  ctx.fillStyle = C.BG; ctx.fillRect(0, 0, 320, 480);
  ctx.textBaseline = 'top'; ctx.textAlign = 'left';
  statusBar(ctx, s.label, s.sub, i, SCENES.length);
  s.body(ctx, t);
  hintBar(ctx, s.hint[0], s.hint[1]);
}

export function drawScreen(c: HTMLCanvasElement, t: number) {
  const ctx = c.getContext('2d');
  if (!ctx) return;
  const w = c.width, h = c.height;
  ctx.setTransform(1, 0, 0, 1, 0, 0);
  ctx.globalAlpha = 1;
  ctx.fillStyle = '#04050a'; // ink border under the glass
  ctx.fillRect(0, 0, w, h);

  const ax = ((SCREEN_W - ACT_W) / 2) * PX;
  const topMm = SCREEN_CY + SCREEN_H / 2 - (ACT_CY + ACT_H / 2);
  const ay = topMm * PX;
  const aw = ACT_W * PX, ah = ACT_H * PX;

  ctx.save();
  ctx.beginPath(); ctx.rect(ax, ay, aw, ah); ctx.clip();
  ctx.translate(ax, ay);
  ctx.scale(aw / 320, ah / 480); // from here on: 320×480 device pixels

  const n = SCENES.length;
  const k = Math.floor(t / SCENE_DUR);
  const cur = ((k % n) + n) % n, local = t - k * SCENE_DUR;
  drawScene(ctx, cur, t);
  if (local > SCENE_DUR - FADE) {
    ctx.globalAlpha = (local - (SCENE_DUR - FADE)) / FADE;
    drawScene(ctx, (cur + 1) % n, t);
    ctx.globalAlpha = 1;
  }
  ctx.restore();
}

/* ---------------- Status-light legend timeline ---------------- */

const LEGEND = [
  { c: 0x4da3ff, dur: 3.0, mode: 'blink' },  // sending / awaiting confirmation
  { c: 0x14f195, dur: 3.0, mode: 'solid' },  // confirmed 3 s
  { c: 0xff4d4d, dur: 2.2, mode: 'triple' }, // failed
  { c: 0xff4d4d, dur: 3.0, mode: 'slow5' },  // battery < 10 %
  { c: 0xffb020, dur: 3.0, mode: 'solid' },  // charging
  { c: 0x14f195, dur: 2.0, mode: 'solid' },  // full
  { c: 0x9945ff, dur: 4.0, mode: 'slow' },   // AI proposal waiting
];
const LEGEND_TOTAL = LEGEND.reduce((a, s) => a + s.dur, 0);

export function legendAt(t: number): { color: number; k: number } {
  let u = t % LEGEND_TOTAL;
  for (const s of LEGEND) {
    if (u < s.dur) {
      let k = 1;
      if (s.mode === 'blink') k = u % 0.8 < 0.4 ? 1 : 0.08;
      else if (s.mode === 'triple') k = u < 1.2 && u % 0.4 < 0.2 ? 1 : 0.08;
      else if (s.mode === 'slow5') k = u % 1.5 < 0.25 ? 1 : 0.08;
      else if (s.mode === 'slow') k = 0.25 + 0.75 * (0.5 + 0.5 * Math.sin(u * 2.4));
      return { color: s.c, k };
    }
    u -= s.dur;
  }
  return { color: 0x14f195, k: 1 };
}

/* ---------------- Materials ---------------- */

function addRim<T extends THREE.MeshStandardMaterial>(mat: T, rimColor: number, strength: number): T {
  mat.onBeforeCompile = (s) => {
    s.uniforms.uRim = { value: new THREE.Color(rimColor) };
    s.uniforms.uRimK = { value: strength };
    s.fragmentShader = s.fragmentShader
      .replace('#include <common>', '#include <common>\nuniform vec3 uRim;\nuniform float uRimK;')
      .replace(
        '#include <dithering_fragment>',
        'float rimF = pow(1.0 - saturate(dot(normalize(vNormal), normalize(vViewPosition))), 3.0);\n' +
          'gl_FragColor.rgb += uRim * rimF * uRimK;\n#include <dithering_fragment>',
      );
  };
  mat.customProgramCacheKey = () => 'dw-rim';
  return mat;
}

export interface Materials {
  shell: THREE.MeshPhysicalMaterial;
  back: THREE.MeshPhysicalMaterial;
  key: THREE.MeshStandardMaterial;
  trade: THREE.MeshStandardMaterial;
  wheel: THREE.MeshStandardMaterial;
  clip: THREE.MeshPhysicalMaterial;
  light: THREE.MeshStandardMaterial;
  gold: THREE.MeshStandardMaterial;
  glassDark: THREE.MeshStandardMaterial;
  lens: THREE.MeshStandardMaterial;
  bezel: THREE.MeshStandardMaterial;
  inner: THREE.MeshStandardMaterial;
  edge: THREE.LineBasicMaterial;
  edgeClip: THREE.LineBasicMaterial;
}

/* One black slab: shell and back are the same matte black (#121417) so the 1.6 mm rim merges with the glass's ink border.
   A soft rim-light term and faint edge lines keep the silhouette, wheel and keys readable. */
export function makeMaterials(): Materials {
  // Front shell: same black as the glass ink (#04050a) and almost no rim/env so the 1.6 mm frame disappears into the screen's black border.
  const shell = addRim(new THREE.MeshPhysicalMaterial({ color: 0x06070a, roughness: 0.8, metalness: 0.02, clearcoat: 0.04, clearcoatRoughness: 0.8, envMapIntensity: 0.18 }), 0x6b7382, 0.06);
  const back = addRim(new THREE.MeshPhysicalMaterial({ color: 0x121417, roughness: 0.5, metalness: 0.08, clearcoat: 0.3, clearcoatRoughness: 0.4, envMapIntensity: 0.65 }), 0x8a96a8, 0.22);
  return {
    shell,
    back,
    key: new THREE.MeshStandardMaterial({ color: 0x1a1d21, roughness: 0.5, metalness: 0.15, envMapIntensity: 0.7 }),
    trade: new THREE.MeshStandardMaterial({ color: NEON, roughness: 0.42, metalness: 0.0, emissive: NEON, emissiveIntensity: 0.14 }),
    wheel: new THREE.MeshStandardMaterial({ color: 0x2a2e33, roughness: 0.3, metalness: 0.85, envMapIntensity: 1.1 }),
    clip: addRim(new THREE.MeshPhysicalMaterial({ color: 0x121417, roughness: 0.6, metalness: 0.06, clearcoat: 0.2, clearcoatRoughness: 0.5, envMapIntensity: 0.55 }), 0xb49bff, 0.26),
    light: new THREE.MeshStandardMaterial({ color: 0xdfffee, emissive: NEON, emissiveIntensity: 0.9, roughness: 0.3, metalness: 0.0, transparent: true, opacity: 0.95 }),
    gold: new THREE.MeshStandardMaterial({ color: 0xe3b341, roughness: 0.28, metalness: 0.95, envMapIntensity: 1.2 }),
    glassDark: new THREE.MeshStandardMaterial({ color: 0x090b10, roughness: 0.06, metalness: 0.4, envMapIntensity: 1.0 }),
    lens: new THREE.MeshStandardMaterial({ color: 0x18264a, roughness: 0.1, metalness: 0.3, emissive: 0x0b1a3a, emissiveIntensity: 0.4 }),
    bezel: new THREE.MeshStandardMaterial({ color: 0x4a5058, roughness: 0.35, metalness: 0.7 }),
    inner: new THREE.MeshStandardMaterial({ color: 0x090a0c, roughness: 1.0, metalness: 0.0 }),
    edge: new THREE.LineBasicMaterial({ color: 0x8e98a6, transparent: true, opacity: 0.2 }),
    edgeClip: new THREE.LineBasicMaterial({ color: 0xa78bff, transparent: true, opacity: 0.2 }),
  };
}

function basisMatrix(basis: Basis, pos: Vec3): THREE.Matrix4 {
  const m = new THREE.Matrix4();
  const bx = new THREE.Vector3(...basis[0]);
  const by = new THREE.Vector3(...basis[1]);
  const bz = new THREE.Vector3(...basis[2]);
  const cross = new THREE.Vector3().crossVectors(bx, by);
  if (cross.distanceTo(bz) > 1e-6) throw new Error('left-handed basis for part placement');
  m.makeBasis(bx, by, bz);
  m.setPosition(pos[0], pos[1], pos[2]);
  return m;
}

function glowTexture(): THREE.CanvasTexture {
  const c = document.createElement('canvas');
  c.width = c.height = 128;
  const g = c.getContext('2d')!;
  const grad = g.createRadialGradient(64, 64, 0, 64, 64, 64);
  grad.addColorStop(0, 'rgba(255,255,255,0.9)');
  grad.addColorStop(0.35, 'rgba(255,255,255,0.35)');
  grad.addColorStop(1, 'rgba(255,255,255,0)');
  g.fillStyle = grad; g.fillRect(0, 0, 128, 128);
  const tex = new THREE.CanvasTexture(c);
  tex.colorSpace = THREE.SRGBColorSpace;
  return tex;
}

function ringTexture(): THREE.CanvasTexture {
  const c = document.createElement('canvas');
  c.width = c.height = 128;
  const g = c.getContext('2d')!;
  g.strokeStyle = '#14f195'; g.lineWidth = 8;
  g.beginPath(); g.arc(64, 64, 50, 0, Math.PI * 2); g.stroke();
  g.fillStyle = '#14f195';
  g.beginPath(); g.arc(64, 64, 12, 0, Math.PI * 2); g.fill();
  const tex = new THREE.CanvasTexture(c);
  tex.colorSpace = THREE.SRGBColorSpace;
  return tex;
}

/* ---------------- Geometry cache (shared across viewers on a page) ---------------- */

let geoPromise: Promise<Map<string, THREE.BufferGeometry>> | null = null;

export function loadGeometries(onProgress?: (n: number, total: number) => void): Promise<Map<string, THREE.BufferGeometry>> {
  if (geoPromise) return geoPromise;
  const loader = new STLLoader();
  let loaded = 0;
  const cache = new Map<string, THREE.BufferGeometry>();
  geoPromise = Promise.all(
    MODEL_FILES.map(async (file) => {
      const geo = await loader.loadAsync(`/models/${file}.stl?v=${MODEL_VERSION}`);
      loaded += 1;
      onProgress?.(loaded, MODEL_FILES.length);
      cache.set(file, geo);
    }),
  )
    .then(() => cache)
    .catch((e) => {
      geoPromise = null; // allow a retry on the next viewer
      throw e;
    });
  return geoPromise;
}

export function hasWebGL(): boolean {
  try {
    const c = document.createElement('canvas');
    return !!(window.WebGLRenderingContext && (c.getContext('webgl2') || c.getContext('webgl')));
  } catch {
    return false;
  }
}

/* ---------------- Renderer / scene ---------------- */

export function createRenderer(canvas: HTMLCanvasElement): THREE.WebGLRenderer {
  const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: true, powerPreference: 'high-performance' });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
  renderer.setClearColor(0x000000, 0);
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.25;
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  return renderer;
}

export function createScene(renderer: THREE.WebGLRenderer): THREE.Scene {
  const scene = new THREE.Scene();
  const pmrem = new THREE.PMREMGenerator(renderer);
  scene.environment = pmrem.fromScene(new RoomEnvironment(), 0.04).texture;
  pmrem.dispose();
  // Studio: hemisphere + three directionals. The rim light from behind keeps the black slab's silhouette.
  scene.add(new THREE.HemisphereLight(0xffffff, 0x2a3140, 1.1));
  const key = new THREE.DirectionalLight(0xffffff, 2.0); key.position.set(-140, 190, 230); scene.add(key);
  const fill = new THREE.DirectionalLight(0xe4ecff, 0.9); fill.position.set(170, 30, 190); scene.add(fill);
  const rim = new THREE.DirectionalLight(0xffffff, 1.9); rim.position.set(60, 140, -260); scene.add(rim);
  return scene;
}

/* Fit distance for a camera (vertical fov) so a box of fitW × fitH mm fills the view */
export function fitDistance(camera: THREE.PerspectiveCamera, fitW = 76 * 1.04, fitH = 132 * 1.04): number {
  const vfov = THREE.MathUtils.degToRad(camera.fov);
  const distH = fitH / 2 / Math.tan(vfov / 2);
  const distW = fitW / 2 / Math.tan(vfov / 2) / camera.aspect;
  return Math.max(distH, distW);
}

/* ---------------- Device rig ---------------- */

export interface DeviceRig {
  device: THREE.Group;
  clipGroup: THREE.Group;
  pogoGroup: THREE.Group;
  glow: THREE.Sprite;
  marker: THREE.Sprite;
  mats: Materials;
  screenCanvas: HTMLCanvasElement;
  screenTex: THREE.CanvasTexture;
  setLight(color: number, k: number): void;
  redrawScreen(t: number): void;
  dispose(): void;
}

export function buildDevice(cache: Map<string, THREE.BufferGeometry>, renderer: THREE.WebGLRenderer): DeviceRig {
  const device = new THREE.Group();
  const mats = makeMaterials();
  const disposables: { dispose(): void }[] = [];

  // Screen glass with the Room UI as an emissive map
  const screenCanvas = makeScreenCanvas();
  drawScreen(screenCanvas, 0);
  const screenTex = new THREE.CanvasTexture(screenCanvas);
  screenTex.colorSpace = THREE.SRGBColorSpace;
  screenTex.anisotropy = Math.min(8, renderer.capabilities.getMaxAnisotropy());
  const glassMat = new THREE.MeshStandardMaterial({ color: 0x000000, roughness: 0.1, metalness: 0.0, emissive: 0xffffff, emissiveMap: screenTex, emissiveIntensity: 1.0, envMapIntensity: 0.7 });
  const glassGeo = new THREE.PlaneGeometry(SCREEN_W, SCREEN_H);
  const glass = new THREE.Mesh(glassGeo, glassMat);
  glass.position.set(0, SCREEN_CY, 0.05);
  device.add(glass);
  disposables.push(glassGeo, glassMat, screenTex);

  // Black front face: an unlit rounded plate over the whole front (inset 0.5 mm from the outer edge) so the
  // frame around the screen reads as the same black as the glass ink border, whatever the lighting does.
  const FW = 64.5 - 1.0, FH = H - 1.0, FR = 3.5;
  const faceShape = new THREE.Shape();
  faceShape.moveTo(-FW / 2 + FR, -FH / 2);
  faceShape.lineTo(FW / 2 - FR, -FH / 2);
  faceShape.quadraticCurveTo(FW / 2, -FH / 2, FW / 2, -FH / 2 + FR);
  faceShape.lineTo(FW / 2, FH / 2 - FR);
  faceShape.quadraticCurveTo(FW / 2, FH / 2, FW / 2 - FR, FH / 2);
  faceShape.lineTo(-FW / 2 + FR, FH / 2);
  faceShape.quadraticCurveTo(-FW / 2, FH / 2, -FW / 2, FH / 2 - FR);
  faceShape.lineTo(-FW / 2, -FH / 2 + FR);
  faceShape.quadraticCurveTo(-FW / 2, -FH / 2, -FW / 2 + FR, -FH / 2);
  const faceGeo = new THREE.ShapeGeometry(faceShape);
  const faceMat = new THREE.MeshBasicMaterial({ color: 0x04050a });
  const face = new THREE.Mesh(faceGeo, faceMat);
  face.position.set(0, Y_MID, 0.03);
  device.add(face);
  disposables.push(faceGeo, faceMat);

  // Dark interior so holes (grille, mic, slots) read dark instead of see-through
  const innerGeo = new THREE.BoxGeometry(59, 105, 6);
  const inner = new THREE.Mesh(innerGeo, mats.inner);
  inner.position.set(0, -2, -4.8);
  device.add(inner);
  disposables.push(innerGeo);

  // Gold pogo contacts (the STL only has the holes)
  const pogoGroup = new THREE.Group();
  const discGeo = new THREE.CylinderGeometry(1.15, 1.15, 1.5, 24);
  discGeo.rotateX(Math.PI / 2);
  for (let i = 0; i < POGO.n; i++) {
    const m = new THREE.Mesh(discGeo, mats.gold);
    m.position.set(POGO.x0 + POGO.pitch * i, POGO.y, -9.3);
    pogoGroup.add(m);
  }
  device.add(pogoGroup);
  disposables.push(discGeo);

  // Camera window: dark glass + lens + bezel ring, facing -z
  const camGroup = new THREE.Group();
  const winGeo = new THREE.CircleGeometry(CAM.d / 2 - 0.2, 40);
  const lensGeo = new THREE.CircleGeometry(1.5, 32);
  const bezGeo = new THREE.RingGeometry(CAM.d / 2 - 0.9, CAM.d / 2 - 0.15, 40);
  const win = new THREE.Mesh(winGeo, mats.glassDark);
  const lens = new THREE.Mesh(lensGeo, mats.lens);
  const bez = new THREE.Mesh(bezGeo, mats.bezel);
  win.position.z = -9.98; lens.position.z = -10.04; bez.position.z = -10.03;
  for (const m of [win, lens, bez]) { m.rotation.y = Math.PI; camGroup.add(m); }
  camGroup.position.set(CAM.x, CAM.y, 0);
  device.add(camGroup);
  disposables.push(winGeo, lensGeo, bezGeo);

  // Status-light glow sprite at the nub tip
  const glowMat = new THREE.SpriteMaterial({ map: glowTexture(), color: NEON, transparent: true, opacity: 0.85, blending: THREE.AdditiveBlending, depthWrite: false });
  const glow = new THREE.Sprite(glowMat);
  glow.scale.set(16, 16, 1);
  glow.position.set(-9, Y_TOP + 8.5, -5);
  device.add(glow);
  disposables.push(glowMat);

  // Feature marker (hidden until a callout is active)
  const markerMat = new THREE.SpriteMaterial({ map: ringTexture(), transparent: true, depthTest: false, depthWrite: false });
  const marker = new THREE.Sprite(markerMat);
  marker.scale.set(9, 9, 1);
  marker.visible = false;
  device.add(marker);
  disposables.push(markerMat);

  const clipGroup = new THREE.Group();
  device.add(clipGroup);

  for (const p of PARTS) {
    const base = cache.get(p.file);
    if (!base) continue;
    const geo = p.basis ? base.clone() : base;
    if (p.basis && p.pos) { geo.applyMatrix4(basisMatrix(p.basis, p.pos)); disposables.push(geo); }
    const mesh = new THREE.Mesh(geo, mats[p.mat]);
    const parent = p.clip ? clipGroup : device;
    parent.add(mesh);
    if (p.edges && p.file !== 'front_shell') {
      const eg = new THREE.EdgesGeometry(geo, 38);
      parent.add(new THREE.LineSegments(eg, p.clip ? mats.edgeClip : mats.edge));
      disposables.push(eg);
    }
  }

  const lightCol = new THREE.Color();
  return {
    device,
    clipGroup,
    pogoGroup,
    glow,
    marker,
    mats,
    screenCanvas,
    screenTex,
    setLight(color, k) {
      lightCol.setHex(color);
      mats.light.emissive.copy(lightCol);
      mats.light.emissiveIntensity = 0.25 + 0.9 * k;
      glowMat.color.copy(lightCol);
      glowMat.opacity = 0.15 + 0.75 * k;
    },
    redrawScreen(t) {
      drawScreen(screenCanvas, t);
      screenTex.needsUpdate = true;
    },
    dispose() {
      for (const d of disposables) d.dispose();
      for (const m of Object.values(mats)) (m as THREE.Material).dispose();
    },
  };
}
