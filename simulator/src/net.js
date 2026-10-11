// In-page network: the firmware's HTTP and WebSocket calls are routed here by host.
// Each mock registers a base URL. Everything is logged for the "Network" panel.

const routes = [];   // { base, http(method, path, query, headers, body) -> {status, body}, ws(path, socket) }
const listeners = new Set();
let seq = 0;

export function register(base, handlers) { routes.push({ base, ...handlers }); }
export function onLog(fn) { listeners.add(fn); return () => listeners.delete(fn); }
export function log(entry) {
  const e = { id: ++seq, t: Date.now(), ...entry };
  for (const fn of listeners) fn(e);
  return e;
}

function find(url) {
  return routes.find((r) => url.startsWith(r.base) || (r.wsBase && url.startsWith(r.wsBase)));
}
const baseOf = (r, url) => (r.wsBase && url.startsWith(r.wsBase) ? r.wsBase : r.base);

const latency = () => 60 + Math.random() * 140;

export async function http(method, url, headers, body) {
  const r = find(url);
  const u = new URL(url);
  const path = url.slice(r ? r.base.length : 0).split('?')[0] || '/';
  const entry = { kind: 'http', method, url, host: u.host, path: u.pathname, req: body, notes: [] };
  if (!r || !r.http) {
    log({ ...entry, status: 0, res: 'no route' });
    return { status: -1, body: '' };
  }
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

/* A WebSocket-shaped object backed by a mock. */
export function socket(url, onMessage, onState) {
  const r = find(url);
  const u = new URL(url);
  const s = {
    open: false,
    send(text) {
      log({ kind: 'ws-out', url, host: u.host, path: u.pathname, req: text });
      if (s.open && s.onClientMessage) setTimeout(() => s.onClientMessage(text), 20);
    },
    push(obj, quiet) {
      if (!s.open) return;
      const text = typeof obj === 'string' ? obj : JSON.stringify(obj);
      if (!quiet) log({ kind: 'ws-in', url, host: u.host, path: u.pathname, res: text });
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
