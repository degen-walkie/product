// The firmware's network. Demo mode routes every call to the in-page mocks; real mode
// sends it to the real venue (testnet or mainnet) and to the relay. Everything is
// logged for the Network panel either way.

const routes = [];   // mocks: { base, wsBase, http(...), ws(path, socket) }
const listeners = new Set();
let seq = 0;
let real = false;

export function setReal(on) { real = on; }
export function register(base, handlers) { routes.push({ base, ...handlers }); }
export function onLog(fn) { listeners.add(fn); return () => listeners.delete(fn); }
export function log(entry) {
  const e = { id: ++seq, t: Date.now(), real, ...entry };
  for (const fn of listeners) fn(e);
  return e;
}

function find(url) {
  return routes.find((r) => url.startsWith(r.base) || (r.wsBase && url.startsWith(r.wsBase)));
}
const baseOf = (r, url) => (r.wsBase && url.startsWith(r.wsBase) ? r.wsBase : r.base);
const latency = () => 60 + Math.random() * 140;

function parseHeaders(text) {
  const h = {};
  for (const line of (text || '').split('\n')) {
    const i = line.indexOf(':');
    if (i > 0 && !/^x-from$/i.test(line.slice(0, i).trim())) h[line.slice(0, i).trim()] = line.slice(i + 1).trim();
  }
  return h;
}

async function realHttp(method, url, headers, body, entry) {
  try {
    const r = await fetch(url, { method, headers: parseHeaders(headers), body: body ?? undefined });
    const text = await r.text();
    log({ ...entry, status: r.status, res: text });
    return { status: r.status, body: text };
  } catch (e) {
    log({ ...entry, status: 0, res: String(e && e.message || e) });
    return { status: -1, body: '' };
  }
}

export async function http(method, url, headers, body) {
  headers = (headers || '').split('\n').filter((l) => l && !l.startsWith('X-DW-Lane')).join('\n');   // the firmware's lane marker
  const u = new URL(url);
  const entry = { kind: 'http', method, url, host: u.host, path: u.pathname, req: body, notes: [] };
  if (real) return realHttp(method, url, headers, body, entry);
  const r = find(url);
  if (!r || !r.http) {
    log({ ...entry, status: 0, res: 'no route' });
    return { status: -1, body: '' };
  }
  const path = url.slice(r.base.length).split('?')[0] || '/';
  await new Promise((res) => setTimeout(res, latency()));
  let out;
  try {
    out = await r.http(method, path, Object.fromEntries(u.searchParams), headers || '', body, (n) => entry.notes.push(n));
  } catch (e) {
    console.error(e);
    out = { status: 500, body: { error: String(e && e.message || e) } };
  }
  const text = typeof out.body === 'string' ? out.body : JSON.stringify(out.body);
  log({ ...entry, status: out.status, res: text });
  return { status: out.status, body: text };
}

/* A WebSocket-shaped object: a real socket in real mode, a mock in demo mode. */
export function socket(url, onMessage, onState) {
  const u = new URL(url);
  const meta = { url, host: u.host, path: u.pathname };
  if (real) return realSocket(url, onMessage, onState, meta);
  const r = find(url);
  const s = {
    open: false,
    send(text) {
      log({ kind: 'ws-out', ...meta, req: text });
      if (s.open && s.onClientMessage) setTimeout(() => s.onClientMessage(text), 20);
    },
    push(obj, quiet) {
      if (!s.open) return;
      const text = typeof obj === 'string' ? obj : JSON.stringify(obj);
      if (!quiet) log({ kind: 'ws-in', ...meta, res: text });
      onMessage(text);
    },
    close() { s.open = false; if (s.onClose) s.onClose(); onState(0); },
  };
  setTimeout(() => {
    if (!r || !r.ws) { onState(0); return; }
    s.open = true;
    r.ws(url.slice(baseOf(r, url).length), s);
    onState(1);
  }, latency());
  return s;
}

function realSocket(url, onMessage, onState, meta) {
  let ws, closed = false, retry = 1000;
  const s = {
    send(text) {
      log({ kind: 'ws-out', ...meta, req: text });
      if (ws && ws.readyState === 1) ws.send(text);
    },
    close() { closed = true; if (ws) ws.close(); },
  };
  const connect = () => {
    ws = new WebSocket(url);
    ws.onopen = () => { retry = 1000; onState(1); };
    ws.onmessage = (e) => { if (typeof e.data === 'string') onMessage(e.data); };
    ws.onclose = () => {
      onState(0);
      if (!closed) setTimeout(connect, retry = Math.min(retry * 2, 15000));
    };
    ws.onerror = () => {};
  };
  connect();
  // Hyperliquid closes idle sockets; keep it alive.
  setInterval(() => { if (ws && ws.readyState === 1 && /hyperliquid|pacifica/.test(url)) ws.send('{"method":"ping"}'); }, 30000);
  return s;
}
