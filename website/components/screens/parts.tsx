'use client';

import { useEffect, useRef, useState, type ReactNode } from 'react';

/* Primitives for the 320×480 on-device UI, drawn in HTML inside the CSS device frame (1em = 10 device px). */

type IconName = 'wifi' | 'bat' | 'bolt' | 'chev' | 'check' | 'qr' | 'lock' | 'wheel' | 'bot' | 'globe' | 'down' | 'key' | 'mic' | 'x' | 'copy' | 'power' | 'chart' | 'perps' | 'meme' | 'chat' | 'assets' | 'gear' | 'radio';

const PATHS: Record<IconName, ReactNode> = {
  wifi: <><path d="M2 9a15 15 0 0 1 20 0" /><path d="M5.5 12.5a10 10 0 0 1 13 0" /><path d="M9 16a5 5 0 0 1 6 0" /><circle cx="12" cy="19.5" r="1" fill="currentColor" /></>,
  bat: <><rect x="2" y="7" width="18" height="10" rx="2" /><path d="M22 10v4" /><rect x="4" y="9" width="12" height="6" fill="currentColor" stroke="none" /></>,
  bolt: <path d="M13 2L4 14h7l-1 8 9-12h-7l1-8z" />,
  chev: <path d="M9 6l6 6-6 6" />,
  check: <path d="M4 12l5 5L20 7" />,
  qr: <><rect x="3" y="3" width="7" height="7" /><rect x="14" y="3" width="7" height="7" /><rect x="3" y="14" width="7" height="7" /><path d="M14 14h3v3h-3zM20 14v2M14 20h2M19 19h2v2" /></>,
  lock: <><rect x="5" y="11" width="14" height="10" rx="2" /><path d="M8 11V7a4 4 0 0 1 8 0v4" /></>,
  wheel: <><circle cx="12" cy="12" r="8" /><path d="M12 4v2M12 18v2M4 12h2M18 12h2M6.3 6.3l1.4 1.4M16.3 16.3l1.4 1.4M6.3 17.7l1.4-1.4M16.3 7.7l1.4-1.4" /></>,
  bot: <><rect x="4" y="8" width="16" height="11" rx="2" /><path d="M12 4v4M8 19v2M16 19v2" /><circle cx="9" cy="13" r="1" fill="currentColor" /><circle cx="15" cy="13" r="1" fill="currentColor" /></>,
  globe: <><circle cx="12" cy="12" r="9" /><path d="M3 12h18M12 3a14 14 0 0 1 0 18M12 3a14 14 0 0 0 0 18" /></>,
  down: <path d="M12 5v14M6 13l6 6 6-6" />,
  key: <><rect x="3" y="9" width="5" height="6" rx="1" /><path d="M8 12h13M18 12v3M15 12v2" /></>,
  mic: <><rect x="9" y="3" width="6" height="11" rx="3" /><path d="M5 11a7 7 0 0 0 14 0M12 18v3" /></>,
  x: <path d="M6 6l12 12M18 6L6 18" />,
  copy: <><rect x="9" y="9" width="11" height="11" rx="2" /><path d="M5 15V5a1 1 0 0 1 1-1h10" /></>,
  power: <><path d="M12 3v9" /><path d="M6.3 7.3a8 8 0 1 0 11.4 0" /></>,
  chart: <path d="M4 19l5-6 4 3 7-9M4 19h16" />,
  perps: <path d="M4 17l4-4 4 2 8-8M14 7h6v6" />,
  meme: <><circle cx="12" cy="12" r="9" /><path d="M8 10h.01M16 10h.01M8 15c1 1.5 2.5 2 4 2s3-.5 4-2" /></>,
  chat: <path d="M4 5h16v10H9l-5 4V5z" />,
  assets: <><rect x="3" y="6" width="18" height="12" rx="2" /><path d="M3 10h18M15 15h3" /></>,
  gear: <><circle cx="12" cy="12" r="3" /><path d="M12 2v3M12 19v3M2 12h3M19 12h3M4.9 4.9l2.1 2.1M17 17l2.1 2.1M4.9 19.1L7 17M17 7l2.1-2.1" /></>,
  radio: <><path d="M4 10v4h3l4 3V7l-4 3H4z" /><path d="M15 9a4 4 0 0 1 0 6" /><path d="M17.5 6.5a8 8 0 0 1 0 11" /></>,
};

export function Icon({ name, className, style }: { name: IconName; className?: string; style?: React.CSSProperties }) {
  return (
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true" className={className} style={style}>
      {PATHS[name]}
    </svg>
  );
}

/* Status bar */
export function Bar({ left, right, bat = 86, charging = false }: { left: ReactNode; right?: ReactNode; bat?: number; charging?: boolean }) {
  return (
    <div className="ui-bar">
      <div className="l">{left}</div>
      <div className="r">
        {right ?? (
          <>
            {charging ? <Icon name="bolt" /> : <Icon name="wifi" />}
            <Icon name="bat" />
            <span>{bat}</span>
          </>
        )}
      </div>
    </div>
  );
}

/* The 3-slot hint bar — how users learn the controls. Pass null/undefined for a slot that does nothing. */
export function Hint({ roll, press, trade, sign }: { roll?: string | null; press?: string | null; trade?: string | null; sign?: boolean }) {
  return (
    <div className="ui-hint" aria-label="Control hints">
      <div className={`hs${roll ? '' : ' off'}`}>
        <i><WheelMini /> roll</i>
        <b>{roll || '—'}</b>
      </div>
      <div className={`hs${press ? '' : ' off'}`}>
        <i><PressMini /> press</i>
        <b>{press || '—'}</b>
      </div>
      <div className={`hs trade${sign ? ' sign' : ''}${trade ? '' : ' off'}`}>
        <i><KeyMini /> trade</i>
        <b>{trade || '—'}</b>
      </div>
    </div>
  );
}

function WheelMini() {
  return <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.2" aria-hidden="true"><path d="M20 12a8 8 0 1 1-2.3-5.7" /><path d="M20 4v4h-4" /></svg>;
}
function PressMini() {
  return <svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><circle cx="12" cy="12" r="5" /><circle cx="12" cy="12" r="9" fill="none" stroke="currentColor" strokeWidth="1.6" /></svg>;
}
function KeyMini() {
  return <svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><rect x="8" y="4" width="8" height="16" rx="2" /></svg>;
}

/* List row. `sel` marks it as THE focus. */
export function Row({ title, sub, val, small, sel, mono = true, chev, md, ok }: { title: ReactNode; sub?: ReactNode; val?: ReactNode; small?: ReactNode; sel?: boolean; mono?: boolean; chev?: boolean; md?: boolean; ok?: boolean }) {
  return (
    <div className={`ui-row${sel ? ' sel' : ''}`}>
      <div>
        <div className={`t${mono ? ' mono' : ''}${md ? ' md' : ''}`}>{title}</div>
        {sub ? <div className="s">{sub}</div> : null}
      </div>
      {val !== undefined ? (
        <div className="v">
          {val}
          {small ? <small>{small}</small> : null}
        </div>
      ) : null}
      {chev ? <Icon name="chev" className="chev" /> : null}
      {ok ? <Icon name="check" className="ok" /> : null}
    </div>
  );
}

export function Msg({ who, lang, text, me }: { who: string; lang: string; text: string; me?: boolean }) {
  return (
    <div className={`ui-msg${me ? ' me' : ''}`}>
      <div className="who">
        {who}
        <span>{lang}</span>
      </div>
      <div className="txt">{text}</div>
    </div>
  );
}

export function Ptt({ label = 'HOLD', sub = 'TO TALK', muted }: { label?: string; sub?: string; muted?: boolean }) {
  return (
    <div className={`ui-ptt${muted ? ' muted' : ''}`}>
      <div>
        <div className="ui-wave">{Array.from({ length: 9 }, (_, i) => <i key={i} />)}</div>
        <b>{label}</b>
        <small>{sub}</small>
      </div>
    </div>
  );
}

export function Chart({ tall }: { tall?: boolean }) {
  return (
    <div className={`ui-chart${tall ? ' tall' : ''}`}>
      <svg viewBox="0 0 300 90" preserveAspectRatio="none" aria-hidden="true">
        <polyline fill="none" stroke="#14f195" strokeWidth="2" points="0,70 25,66 50,58 75,62 100,50 125,54 150,40 175,44 200,30 225,36 250,22 275,26 300,14" />
        <polyline fill="rgba(20,241,149,.12)" stroke="none" points="0,90 0,70 25,66 50,58 75,62 100,50 125,54 150,40 175,44 200,30 225,36 250,22 275,26 300,14 300,90" />
      </svg>
    </div>
  );
}

export function Kbd({ keys, on }: { keys: string; on?: string }) {
  return (
    <div className="ui-kbd">
      {keys.split('').map((k, i) => (
        <span key={i} className={k === on ? 'on' : ''}>{k}</span>
      ))}
    </div>
  );
}

export function Tabs({ items, on, page }: { items: string[]; on: number; page?: string }) {
  return (
    <div className="ui-tabs">
      {items.map((t, i) => (
        <span key={t} className={i === on ? 'on' : ''}>{t}</span>
      ))}
      <span className="pg">{page ?? `${on + 1} / ${items.length}`}</span>
    </div>
  );
}

export function Caret() {
  return <span className="ui-caret" aria-hidden="true" />;
}

const AV_COLORS = ['#14f195', '#c89bff', '#ffb020', '#4da3ff', '#ff2db3', '#e3b341'];
export function Avatar({ name, i }: { name: string; i: number }) {
  const initials = name.replace('@', '').slice(0, 2).toUpperCase();
  return <div className="ui-av" style={{ background: AV_COLORS[i % AV_COLORS.length] }}>{initials}</div>;
}

export type LightKind = 'green' | 'blue' | 'purple' | 'yellow' | 'red' | 'off';
export type Press = 'wheel-up' | 'wheel-down' | 'wheel-press' | 'trade' | 'power' | 'power-hold' | null;

/* The device frame: wheel + trade key on the right, power key on the left, light on top (positions from the CAD). */
export function Device({ children, light = 'green', press = null, small, off }: { children: ReactNode; light?: LightKind; press?: Press; small?: boolean; off?: boolean }) {
  return (
    <div className={`device${small ? ' sm' : ''}`} data-press={press ?? undefined}>
      <i className="d-wheel" aria-hidden="true" />
      <i className="d-trade" aria-hidden="true" />
      <i className="d-pwr" aria-hidden="true" />
      <i className={`d-light ${light === 'green' ? '' : light}`} aria-hidden="true" />
      <div className={`scr${off ? ' off' : ''}`}>{children}</div>
    </div>
  );
}

/* Control glyphs beside the animations: a wheel (rotation arrows, hub press) and a key (cap moving). */
export type GlyphState = 'up' | 'down' | 'press' | 'hold' | 'idle';
export function Glyph({ kind, state = 'idle' }: { kind: 'wheel' | 'trade' | 'power'; state?: GlyphState }) {
  if (kind === 'wheel') {
    return (
      <div className={`glyph ${state}`} aria-hidden="true">
        <svg viewBox="0 0 44 44" fill="none" stroke="currentColor" strokeWidth="2">
          <circle className="teeth" cx="22" cy="22" r="15" strokeDasharray="3 2.6" />
          <circle cx="22" cy="22" r="11" strokeOpacity="0.35" />
          <circle className="hub" cx="22" cy="22" r="6" fill="#2c323b" stroke="none" />
          <g className="arrows" strokeWidth="2.2">
            <path d="M22 1.5a20.5 20.5 0 0 1 17 9" />
            <path d="M39 4v7h-7" />
            <path d="M22 42.5a20.5 20.5 0 0 1-17-9" />
            <path d="M5 40v-7h7" />
          </g>
        </svg>
      </div>
    );
  }
  return (
    <div className={`glyph key ${kind === 'power' ? 'pwr ' : ''}${state}`} aria-hidden="true">
      <svg viewBox="0 0 44 44" fill="none" stroke="currentColor" strokeWidth="2">
        <path d="M30 4v36" strokeOpacity="0.5" />
        <path d="M30 4h8v36h-8" strokeOpacity="0.25" />
        <rect className="cap" x="12" y="12" width="12" height="20" rx="3" fill="#2c323b" stroke="none" />
      </svg>
    </div>
  );
}

/* Timeline: advances through step durations (ms) while `running`; frozen at `rest` when reduced motion is on. */
export function useTimeline(durations: number[], running: boolean, rest = 0): number {
  const [i, setI] = useState(rest);
  const reduce = useReducedMotion();
  useEffect(() => {
    if (!running || reduce) return;
    const id = setTimeout(() => setI((x) => (x + 1) % durations.length), durations[i] ?? 1000);
    return () => clearTimeout(id);
  }, [i, running, reduce, durations]);
  return reduce ? rest : i;
}

export function useReducedMotion(): boolean {
  const [r, setR] = useState(false);
  useEffect(() => {
    const mq = window.matchMedia?.('(prefers-reduced-motion: reduce)');
    if (!mq) return;
    setR(mq.matches);
    const fn = (e: MediaQueryListEvent) => setR(e.matches);
    mq.addEventListener?.('change', fn);
    return () => mq.removeEventListener?.('change', fn);
  }, []);
  return r;
}

/* Animations only run while the element is on screen. */
export function useInView<T extends HTMLElement>(): [React.RefObject<T>, boolean] {
  const ref = useRef<T>(null);
  const [inView, setInView] = useState(false);
  useEffect(() => {
    const el = ref.current;
    if (!el) return;
    if (!('IntersectionObserver' in window)) { setInView(true); return; }
    const io = new IntersectionObserver((entries) => setInView(entries[0].isIntersecting), { threshold: 0.15 });
    io.observe(el);
    return () => io.disconnect();
  }, []);
  return [ref, inView];
}
