'use client';

import type { ReactNode } from 'react';
import { Avatar, Bar, Device, Glyph, Hint, Icon, Msg, Ptt, Tabs, useInView, useTimeline, type GlyphState, type LightKind, type Press } from './parts';
import { AccountsScreen, RoomScreen, RoomsScreen, SpotTicket } from './static';

/* Animated demos: each is a looping timeline of steps. Beside the device, a glyph shows which control is
   being used (wheel rolling / pressing, key pressing) and a caption explains the step. */

interface Step { dur: number; press: Press; glyph: 'wheel' | 'trade' | 'power'; state: GlyphState; label: string; cap: string; light?: LightKind }

function AnimShot({ title, text, steps, i, light, press, off, children, small }: { title: string; text: string; steps: Step[]; i: number; light?: LightKind; press: Press; off?: boolean; children: ReactNode; small?: boolean }) {
  const s = steps[i];
  const uniq = steps.reduce<Step[]>((a, st) => (a.find((x) => x.cap === st.cap) ? a : [...a, st]), []);
  return (
    <div className="anim-shot">
      <Device light={light ?? s.light ?? 'green'} press={press} off={off} small={small}>{children}</Device>
      <div className="anim-side">
        <h3>{title}</h3>
        <p>{text}</p>
        <div className="glyph-row">
          <Glyph kind={s.glyph} state={s.state} />
          <div className="glyph-cap"><b>{s.label}</b>{s.cap}</div>
        </div>
        <div className="steps-list" aria-hidden="true">
          {uniq.map((st, k) => (
            <div key={st.cap} className={st.cap === s.cap ? 'on' : ''}><i>{String(k + 1).padStart(2, '0')}</i>{st.cap}</div>
          ))}
        </div>
      </div>
    </div>
  );
}

/* ---------------- 1. Side menu ---------------- */

const MENU = [
  { n: 'Spot', ic: 'chart' },
  { n: 'Perps', ic: 'perps' },
  { n: 'Meme', ic: 'meme' },
  { n: 'Chat', ic: 'chat' },
  { n: 'Accounts', ic: 'assets' },
  { n: 'Settings', ic: 'gear' },
] as const;

export function Drawer({ open, focus, hidden }: { open: boolean; focus: number; hidden?: boolean }) {
  return (
    <>
      <div className={`ui-scrim${open ? ' open' : ''}`} />
      <div className={`ui-drawer${open ? ' open' : ''}`} aria-hidden={hidden || !open}>
        <div className="ui-eyebrow">Menu</div>
        <div className="ui-menu">
          {MENU.map((m, k) => (
            <div key={m.n} className={k === focus ? 'ui-focus' : ''}><Icon name={m.ic} />{m.n}</div>
          ))}
        </div>
        <div className="ui-wsw">
          <div className="ui-eyebrow" style={{ border: 0, padding: 0 }}>Accounts</div>
          <div className="trading on"><span className="wl"><i />Hyperliquid</span><b>$4,210</b></div>
          <div className="vault"><span className="wl"><i />Pacifica</span><b>$1,350</b></div>
        </div>
      </div>
    </>
  );
}

const MENU_STEPS: Step[] = [
  { dur: 1500, press: null, glyph: 'wheel', state: 'idle', label: 'Top level', cap: 'Accounts home. Nothing to go back to.' },
  { dur: 1700, press: 'wheel-press', glyph: 'wheel', state: 'press', label: 'Wheel press · menu', cap: 'Press = back. At the top level there is nothing behind you, so it opens the side menu.' },
  { dur: 900, press: 'wheel-down', glyph: 'wheel', state: 'down', label: 'Roll · move', cap: 'Roll moves the highlight down the menu.' },
  { dur: 900, press: 'wheel-down', glyph: 'wheel', state: 'down', label: 'Roll · move', cap: 'Roll moves the highlight down the menu.' },
  { dur: 1000, press: 'wheel-down', glyph: 'wheel', state: 'down', label: 'Roll · move', cap: 'Roll moves the highlight down the menu.' },
  { dur: 2000, press: 'trade', glyph: 'trade', state: 'press', label: 'Trade key · open', cap: 'Trade key = open. Chat opens the room list; the menu slides away.' },
  { dur: 1400, press: 'wheel-press', glyph: 'wheel', state: 'press', label: 'Wheel press · menu', cap: 'From any top-level screen, press brings the menu back.' },
  { dur: 1600, press: 'wheel-press', glyph: 'wheel', state: 'press', label: 'Wheel press · back', cap: 'Press again closes it. Closing the menu is also a back.' },
];
const MENU_DURS = MENU_STEPS.map((s) => s.dur);

export function MenuDemo() {
  const [ref, inView] = useInView<HTMLDivElement>();
  const i = useTimeline(MENU_DURS, inView, 4);
  const open = i === 1 || i === 2 || i === 3 || i === 4 || i === 6;
  const focus = i <= 1 ? 0 : i === 2 ? 1 : i === 3 ? 2 : 3;
  const onRooms = i >= 5;
  return (
    <div ref={ref}>
      <AnimShot title="Side menu" text="Spot, Perps, Meme, Chat, Accounts, Settings, and your linked accounts pinned at the bottom. The wheel press opens it (a swipe from the left edge does too); the trade key opens the highlighted section." steps={MENU_STEPS} i={i} press={MENU_STEPS[i].press}>
        {onRooms ? <RoomsScreen focus={0} /> : <AccountsScreen />}
        <Drawer open={open} focus={focus} />
        {open ? (
          <div style={{ position: 'absolute', left: 0, right: 0, bottom: 0, zIndex: 8 }}>
            <Hint roll="move" press="close" trade="open" />
          </div>
        ) : null}
      </AnimShot>
    </div>
  );
}

/* ---------------- 2. Wheel navigation: list → room → pages → back ---------------- */

const NAV_STEPS: Step[] = [
  { dur: 1300, press: null, glyph: 'wheel', state: 'idle', label: 'Room list', cap: 'One focus, always: the neon outline.' },
  { dur: 900, press: 'wheel-down', glyph: 'wheel', state: 'down', label: 'Roll · move', cap: 'Rolling moves the focus through the list. Touching a row moves it too.' },
  { dur: 900, press: 'wheel-down', glyph: 'wheel', state: 'down', label: 'Roll · move', cap: 'Rolling moves the focus through the list. Touching a row moves it too.' },
  { dur: 900, press: 'wheel-up', glyph: 'wheel', state: 'up', label: 'Roll · move', cap: 'Rolling moves the focus through the list. Touching a row moves it too.' },
  { dur: 1000, press: 'wheel-up', glyph: 'wheel', state: 'up', label: 'Roll · move', cap: 'Rolling moves the focus through the list. Touching a row moves it too.' },
  { dur: 2000, press: 'trade', glyph: 'trade', state: 'press', label: 'Trade key · open', cap: 'Trade key opens the focused room. Chat page first.' },
  { dur: 1700, press: 'wheel-down', glyph: 'wheel', state: 'down', label: 'Roll · page', cap: 'Inside a room the wheel flips pages: Chat → Chart → Orders.' },
  { dur: 1700, press: 'wheel-down', glyph: 'wheel', state: 'down', label: 'Roll · page', cap: 'Inside a room the wheel flips pages: Chat → Chart → Orders.' },
  { dur: 1700, press: 'wheel-press', glyph: 'wheel', state: 'press', label: 'Wheel press · back', cap: 'Press = back, up one level to the room list.' },
];
const NAV_DURS = NAV_STEPS.map((s) => s.dur);

export function NavDemo() {
  const [ref, inView] = useInView<HTMLDivElement>();
  const i = useTimeline(NAV_DURS, inView, 5);
  const focus = [0, 1, 2, 1, 0][Math.min(i, 4)];
  const inRoom = i >= 5 && i <= 7;
  const page = (i === 6 ? 1 : i === 7 ? 2 : 0) as 0 | 1 | 2;
  return (
    <div ref={ref}>
      <AnimShot title="How the wheel works" text="Roll = move (or adjust a value, or flip a page). Press = back; at the top level it toggles the menu. Trade key = open; on a confirmation sheet it signs. Everything the keys do, touch can do too — the keys are just faster." steps={NAV_STEPS} i={i} press={NAV_STEPS[i].press}>
        {inRoom ? <RoomScreen page={page} /> : <RoomsScreen focus={focus} />}
      </AnimShot>
    </div>
  );
}

/* ---------------- 3. World room = group chat ---------------- */

const CHAT: { who: string; lang: string; og: string; tr?: string; me?: boolean }[] = [
  { who: '@ndiaye', lang: 'FR', og: 'Le funding BTC vient de passer négatif sur deux venues.', tr: 'BTC funding just flipped negative on two venues.' },
  { who: '@hoshi', lang: 'JA', og: '東京オープンまで20分。起きてる人いる？', tr: 'Tokyo open in 20 minutes. Who is awake?' },
  { who: '@mara', lang: 'ES', og: 'Despierta. Mirando #SOL.', tr: 'Awake. Watching #SOL.' },
  { who: '@you', lang: 'EN', og: 'Same. Volume on the Base pool just doubled.', me: true },
  { who: '@ndiaye', lang: 'FR', og: 'Je passe sur #HYPE après ça.', tr: 'Rolling to #HYPE after this.' },
  { who: '@hoshi', lang: 'JA', og: 'ホルダー数が増えてる。', tr: 'Holder count is climbing.' },
  { who: '@mara', lang: 'ES', og: '¿Alguien ve lo mismo en Hyperliquid?', tr: 'Anyone seeing the same on Hyperliquid?' },
];
const SPEAKER_IDX: Record<string, number> = { '@ndiaye': 3, '@hoshi': 2, '@mara': 0, '@you': 1 };
const CHAT_DURS = CHAT.map(() => 2300);

export function WorldScreen({ animate = true }: { animate?: boolean }) {
  const [ref, inView] = useInView<HTMLDivElement>();
  const i = useTimeline(CHAT_DURS, animate && inView, 3);
  const win = [CHAT[(i + CHAT.length - 2) % CHAT.length], CHAT[(i + CHAT.length - 1) % CHAT.length], CHAT[i]];
  return (
    <div ref={ref} style={{ display: 'contents' }}>
      <Bar left={<><span className="room">WORLD</span> <span>· 48,112</span></>} />
      <div className="ui-body tight">
        <div className="ui-head">
          <div>
            <div className="ui-title sm">World</div>
            <div className="ui-sub">EVERYONE · HEARD IN EN</div>
          </div>
          <Icon name="globe" style={{ width: '1.8em', height: '1.8em', color: '#6c7480' }} />
        </div>
        <div className="ui-chat">
          {win.map((m, k) => (
            <div className={`ui-cm${m.me ? ' me' : ''}`} key={`${i}-${k}`}>
              <Avatar name={m.who} i={SPEAKER_IDX[m.who] ?? k} />
              <div className="bub">
                <div className="hd"><b style={{ color: m.me ? '#c89bff' : '#14f195' }}>{m.who}</b><span>{m.lang}{m.tr ? ' → EN' : ''}</span></div>
                <div className="og">{m.og}</div>
                {m.tr ? <div className="tr">{m.tr}</div> : null}
              </div>
            </div>
          ))}
        </div>
        <div className="ui-compose ui-focus" style={{ boxShadow: 'inset 0 0 0 0.2em #14f195' }}>
          <span>Say something…</span>
          <span className="mic"><Icon name="mic" /></span>
        </div>
        <div className="ui-cool"><i /></div>
        <div className="ui-cool-txt">Cooldown · 1 msg / 10 s · new devices listen 24 h</div>
      </div>
      <Hint roll="scroll" press="back" trade="talk" />
    </div>
  );
}

/* ---------------- 4. Spot order → confirmation sheet → signed ---------------- */

const SPOT_STEPS: Step[] = [
  { dur: 1400, press: null, glyph: 'wheel', state: 'idle', label: 'Ticket', cap: 'The focus is on the amount.', light: 'green' },
  { dur: 700, press: 'wheel-up', glyph: 'wheel', state: 'up', label: 'Roll · adjust', cap: 'On a numeric field the wheel changes the value.', light: 'green' },
  { dur: 1000, press: 'wheel-up', glyph: 'wheel', state: 'up', label: 'Roll · adjust', cap: 'On a numeric field the wheel changes the value.', light: 'green' },
  { dur: 2400, press: 'trade', glyph: 'trade', state: 'press', label: 'Trade key · review', cap: 'Trade = open: the confirmation sheet slides up with every detail.', light: 'green' },
  { dur: 1800, press: null, glyph: 'trade', state: 'idle', label: 'Read it', cap: 'Pay, receive, market, fee, signer, account, slippage. Nothing has moved yet.', light: 'green' },
  { dur: 2200, press: 'trade', glyph: 'trade', state: 'press', label: 'Trade key · SIGN', cap: 'Signing is the special enter. The key is the only yes. Blue light while it sends.', light: 'blue' },
  { dur: 1800, press: null, glyph: 'trade', state: 'idle', label: 'Confirmed', cap: 'Green for three seconds.', light: 'green' },
  { dur: 1300, press: 'wheel-press', glyph: 'wheel', state: 'press', label: 'Wheel press · back', cap: 'Press closes the sheet.', light: 'green' },
];
const SPOT_DURS = SPOT_STEPS.map((s) => s.dur);

export function SpotDemo() {
  const [ref, inView] = useInView<HTMLDivElement>();
  const i = useTimeline(SPOT_DURS, inView, 4);
  const amount = i === 0 ? '250.00' : i === 1 ? '260.00' : '270.00';
  const sheet = i >= 3 && i <= 6;
  const signed = i === 5;
  const confirmed = i === 6;
  return (
    <div ref={ref}>
      <AnimShot title="Spot order → confirmation sheet" text="Tapping Review or pressing the trade key opens a confirmation sheet with every detail. The trade key confirms it; the wheel press closes it. The order is signed on the device with a trade-only key." steps={SPOT_STEPS} i={i} press={SPOT_STEPS[i].press}>
        <SpotTicket amount={amount} />
        <div className={`ui-scrim${sheet ? ' open' : ''}`} />
        <div className={`ui-sheet${sheet ? ' open' : ''}`} aria-hidden={!sheet}>
          <div className="grab" />
          <h4>{confirmed ? 'Filled' : signed ? 'Signed · sending' : 'Confirm order'}</h4>
          <dl className="ui-kv">
            <dt>You pay</dt><dd>{amount} USDC<small>Hyperliquid</small></dd>
            <dt>You receive</dt><dd className="neon">≈ 11.17 HYPE</dd>
            <dt></dt><dd className="sub">min. received 11.11 HYPE</dd>
            <dt>Market</dt><dd>HYPE / USDC spot</dd>
            <dt>Fee</dt><dd>≈ 0.09 USDC</dd>
            <dt>Signer</dt><dd>agent 0x8f3…c21<small>trade-only</small></dd>
            <dt>Account</dt><dd>your Hyperliquid account<small>funds stay there</small></dd>
            <dt>Slippage</dt><dd>0.5%</dd>
          </dl>
          {confirmed ? (
            <div className="ui-sign done"><Icon name="check" /> Filled · 11.17 HYPE in your account</div>
          ) : signed ? (
            <div className="ui-sign done"><span className="kg" /> Signed · sending</div>
          ) : (
            <div className="ui-sign"><span className="kg" /> Press the trade key to confirm</div>
          )}
          <Hint roll={null} press={confirmed ? 'done' : 'back'} trade={confirmed || signed ? null : 'confirm'} sign={!signed && !confirmed} />
        </div>
      </AnimShot>
    </div>
  );
}

/* ---------------- 5. Power key: screen off / on, hold for the power-off sheet ---------------- */

const PWR_STEPS: Step[] = [
  { dur: 1400, press: null, glyph: 'power', state: 'idle', label: 'In a room', cap: 'Listening in #SOL.' },
  { dur: 2000, press: 'power', glyph: 'power', state: 'press', label: 'Power · short press', cap: 'Screen off. Radio mode keeps listening; the status light still reports trades.', light: 'green' },
  { dur: 1300, press: 'power', glyph: 'power', state: 'press', label: 'Power · short press', cap: 'Screen back on, where you left it.' },
  { dur: 2000, press: 'power-hold', glyph: 'power', state: 'hold', label: 'Power · hold 2 s', cap: 'Holding two seconds opens the power-off sheet.' },
  { dur: 1800, press: null, glyph: 'wheel', state: 'idle', label: 'Power-off sheet', cap: 'A slide-to-power-off slider and Cancel. The focus is on Cancel.' },
  { dur: 1400, press: 'wheel-press', glyph: 'wheel', state: 'press', label: 'Wheel press · cancel', cap: 'Press = back: the sheet closes. No key can confirm a power-off.' },
  { dur: 1600, press: 'power-hold', glyph: 'power', state: 'hold', label: 'Power · hold 2 s', cap: 'Holding two seconds opens the power-off sheet.' },
  { dur: 1500, press: null, glyph: 'wheel', state: 'idle', label: 'Power-off sheet', cap: 'A slide-to-power-off slider and Cancel. The focus is on Cancel.' },
  { dur: 1500, press: null, glyph: 'power', state: 'idle', label: 'Touch · slide', cap: 'Only a finger on the glass, slid to the end, powers it off.', light: 'off' },
  { dur: 1800, press: null, glyph: 'power', state: 'idle', label: 'Off', cap: 'Goodnight.', light: 'off' },
];
const PWR_DURS = PWR_STEPS.map((s) => s.dur);

export function PowerDemo() {
  const [ref, inView] = useInView<HTMLDivElement>();
  const i = useTimeline(PWR_DURS, inView, 4);
  const off = i === 1 || i === 9;
  const toast = i === 1;
  const sheet = i === 4 || i === 5 || i === 7 || i === 8;
  const go = i === 8;
  const closing = i === 5;
  return (
    <div ref={ref}>
      <AnimShot title="Power key" text="Short press: screen off or on, still listening. Hold two seconds: an iPhone-style sheet with slide to power off and Cancel. The wheel press cancels; the slider must be touched." steps={PWR_STEPS} i={i} press={PWR_STEPS[i].press} off={off}>
        <RoomScreen page={0} />
        <div className={`ui-toast${toast ? ' show' : ''}`}><Icon name="radio" /> Screen off · still listening</div>
        <div className={`ui-pwr-sheet${sheet && !closing ? ' open' : ''}`} aria-hidden={!sheet}>
          <div className="ui-eyebrow c" style={{ color: '#8a929e' }}>Hold power · 2 s</div>
          <div className={`ui-slider${go ? ' go' : ''}`}>
            <span className="knob"><Icon name="power" /></span>
            <span className="ui-shimmer">slide to power off</span>
          </div>
          <div className="ui-cancel ui-focus">Cancel</div>
          <Hint roll={null} press="cancel" trade={null} />
        </div>
      </AnimShot>
    </div>
  );
}

/* Static-looking room with the pulsing PTT, used on /talk alongside the world room. */
export function RoomShot() {
  return (
    <>
      <RoomScreen page={0} />
    </>
  );
}

export { Msg, Ptt, Tabs };
