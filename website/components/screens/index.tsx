'use client';

import type { ReactNode } from 'react';
import { Device, type LightKind } from './parts';
import { AccountsScreen, AgentScreen, AskScreen, VoiceScreen, AssetScreen, ChargingScreen, DiscoverFiltersScreen, DiscoverResultsScreen, LinkScreen, MemeScreen, PerpsScreen, RevokeScreen, RoomScreen, RoomsScreen, SettingsScreen, SpotTicket } from './static';
import { WorldScreen } from './animated';

export { MenuDemo, NavDemo, SpotDemo, PowerDemo, WorldScreen } from './animated';
export { Device } from './parts';

export type ScreenId = 'rooms' | 'room' | 'world' | 'dfilters' | 'dresults' | 'asset' | 'spot' | 'perps' | 'meme' | 'accounts' | 'link' | 'revoke' | 'settings' | 'agent' | 'charging' | 'voice' | 'ask';

export const CAPTIONS: Record<ScreenId, [string, string]> = {
  rooms: ['Room list', 'Roll moves the focus through #SOL, #HYPE, #BTC, #ETH or your private #4471. Trade key opens the room; wheel press opens the menu.'],
  room: ['Room view', 'Ticker header is the default focus, so the trade key opens the SOL ticket. Roll flips Chat / Chart / Orders. Big on-screen mic, radio-mode toggle.'],
  world: ['World room', 'Everyone, one group chat. Avatars, handles, language tags, the original line and its translation. The anti-spam cooldown is the thin bar under the composer.'],
  dfilters: ['Discover · filters', 'Liquidity, holders, age, market cap, chains. Roll adjusts the focused threshold, trade key sets it.'],
  dresults: ['Discover · results', 'Same ticker across chains, side by side, each with its contract address. Trade key opens the one you mean.'],
  asset: ['Asset page', 'Chart / Trade / Holders pages. The header is the focus: trade key opens the HYPE ticket.'],
  spot: ['Spot ticket', 'Hyperliquid spot. Pay, receive, fee, slippage. Roll adjusts the amount; trade key opens the confirmation sheet.'],
  perps: ['Perps ticket', 'Built by touch in a few taps: side, market or limit, leverage, size presets, TP / SL. The wheel fine-tunes any number; the trade key sends it.'],
  meme: ['Meme ticket', 'Memes are covered too, via GMGN. Liquidity and holders on the ticket; trade key opens the confirmation sheet.'],
  accounts: ['Accounts', 'Your money stays on each venue. The device holds trading keys only: Hyperliquid and Pacifica agent keys. Top level: wheel press opens the menu.'],
  link: ['Link an account', 'Approve an agent key from your main wallet in the browser. It can trade; it cannot withdraw or change its own permissions.'],
  revoke: ['Revoke', 'Cut a key off in one press. Lost the device? Revoke from your main wallet or the venue instead; your funds never move.'],
  settings: ['Language', 'Twelve languages. Pinyin, romaji and jamo keyboards built in.'],
  agent: ['AI proposal pending', 'An agent proposed an order over MCP. Purple light. Only your trade key confirms it; wheel press dismisses.'],
  charging: ['Charging · light legend', 'Battery state and what every colour of the status light means.'],
  voice: ['Voice shortcut', 'Hands busy? Say the trade. It only fills in the ticket; nothing is sent until you press the trade key.'],
  ask: ['Ask the AI', 'Same mic. Ask about your positions or the market. Answers only; the AI cannot send an order.'],
};

const LIGHTS: Partial<Record<ScreenId, LightKind>> = { agent: 'purple', charging: 'yellow', revoke: 'blue' };

export function Screen({ id }: { id: ScreenId }) {
  switch (id) {
    case 'rooms': return <RoomsScreen />;
    case 'room': return <RoomScreen />;
    case 'world': return <WorldScreen />;
    case 'dfilters': return <DiscoverFiltersScreen />;
    case 'dresults': return <DiscoverResultsScreen />;
    case 'asset': return <AssetScreen />;
    case 'spot': return <SpotTicket />;
    case 'perps': return <PerpsScreen />;
    case 'meme': return <MemeScreen />;
    case 'accounts': return <AccountsScreen />;
    case 'link': return <LinkScreen />;
    case 'revoke': return <RevokeScreen />;
    case 'settings': return <SettingsScreen />;
    case 'agent': return <AgentScreen />;
    case 'charging': return <ChargingScreen />;
    case 'voice': return <VoiceScreen />;
    case 'ask': return <AskScreen />;
  }
}

/* A device frame with one screen and an optional caption. */
export function Shot({ id, small, caption = true, delay = '', animTag }: { id: ScreenId; small?: boolean; caption?: boolean; delay?: string; animTag?: string }) {
  const [t, s] = CAPTIONS[id];
  return (
    <figure className={`shot rv ${delay}`}>
      <Device light={LIGHTS[id] ?? 'green'} small={small}>
        <Screen id={id} />
      </Device>
      {caption ? (
        <figcaption>
          <b>{t}</b>
          <span>{s}</span>
          {animTag ? <span className="anim-tag">{animTag}</span> : null}
        </figcaption>
      ) : null}
    </figure>
  );
}

export function Pair({ children }: { children: ReactNode }) {
  return <div className="screens pair rv d1">{children}</div>;
}
