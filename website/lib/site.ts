/* Shared product facts and site constants. One place to edit copy that repeats. */

export const SITE_NAME = 'Degen Walkie';
export const PRICE_USD = 99;
export const PRICE_CENTS = 9900;
export const UNITS = 1000;
export const SHIP_DATE = 'January 2027';

export function siteUrl(): string {
  const raw = process.env.NEXT_PUBLIC_SITE_URL || 'http://localhost:3000';
  return raw.replace(/\/+$/, '');
}

export const NAV_LINKS: { href: string; label: string }[] = [
  { href: '/hardware', label: 'Hardware' },
  { href: '/talk', label: 'Rooms' },
  { href: '/trade', label: 'Trade' },
  { href: '/security', label: 'Security' },
  { href: '/screens', label: 'Screens' },
  { href: '/clip', label: '4G Clip' },
  { href: '/roadmap', label: 'Roadmap' },
  { href: '/faq', label: 'FAQ' },
];

/* Phase-1 shipping regions. Value is what we put in Stripe metadata. */
export const PHASE1_REGIONS = [
  'United States',
  'Canada',
  'European Union',
  'United Kingdom',
  'Japan',
  'Hong Kong',
  'Singapore',
] as const;
export const OTHER_REGION = "Other — we'll refund if we can't ship to you";
export const REGION_OPTIONS: readonly string[] = [...PHASE1_REGIONS, OTHER_REGION];

export function isValidRegion(v: unknown): v is string {
  return typeof v === 'string' && REGION_OPTIONS.includes(v);
}

export function isValidEmail(v: unknown): v is string {
  if (typeof v !== 'string') return false;
  const s = v.trim();
  if (s.length < 6 || s.length > 254) return false;
  return /^[^\s@]+@[^\s@]+\.[^\s@]{2,}$/.test(s);
}

export const ROOMS = ['SOL-PERP', 'BTC-PERP', 'ETH-PERP', 'HYPE-PERP', 'SUI-PERP', 'DOGE-PERP', 'XRP-PERP', 'WIF-PERP', 'BNB-PERP', 'AVAX-PERP', 'LINK-PERP', 'ARB-PERP'];

export const DISCLAIMERS = [
  'We never hold your funds. The device holds trade-only keys you can revoke.',
  'Not financial advice. Nothing we show is a recommendation.',
  'Perpetuals only where legal; you confirm your jurisdiction on first boot.',
];

export const FINEPRINT = [
  "Degen Walkie never holds funds and is not a wallet. For perps it holds an agent or API key that your main wallet approved on the venue, and you can revoke it at any time. On Hyperliquid the protocol stops that key from withdrawing; on Lighter it can only withdraw back to your own address; Pacifica's key permissions are being confirmed with Pacifica. For meme coins it holds an API key to your GMGN account: those funds are held by GMGN under GMGN's terms, not by us. Keep only what you actively trade on a venue; large holdings belong in a cold wallet. The ESP32-S3 provides secure boot, encrypted flash and eFuse-protected key storage; it does not include a separate secure element.",
  'Device images are live renders of the Genesis enclosure CAD, not photographs. Final product may differ. Battery, weight and talk-time figures are targets until certification units are measured. Genesis ships after certification; FCC, CE, UKCA, TELEC and IMDA are in progress.',
  'Hyperliquid, Pacifica, Lighter, Robinhood Chain, GMGN and Solana are independent products and networks that Degen Walkie connects to. They are not partners or sponsors; their names belong to their respective owners.',
];

/* Where trades go. Money always stays on the venue; the device holds a key that can trade but not withdraw. */
export const VENUES: { name: string; market: string; how: string; status: 'At launch'; note?: string }[] = [
  { name: 'Hyperliquid', market: 'Perps · spot', how: 'Agent (API) wallet approved by your main wallet. It can trade; the protocol does not let it withdraw. Revoke it from your main wallet any time.', status: 'At launch' },
  { name: 'Pacifica', market: 'Perps · Solana', how: 'Solana-native perps. You generate an agent wallet that signs orders in place of your main key, like an exchange API key, with per-key IP whitelist and one-call revoke.', status: 'At launch', note: 'Key-permission details are still being confirmed with Pacifica.' },
  { name: 'Lighter', market: 'Perps · Robinhood Chain', how: 'The perp DEX of Robinhood Chain. You register an API key with your main wallet; it can trade, and any withdrawal it signs can only go back to your own address. Sending funds anywhere else needs your main wallet.', status: 'At launch', note: 'Integration details are still being confirmed with Lighter.' },
];

export const CERT_LINE = 'FCC, CE, UKCA, TELEC, IMDA in progress';

export const LANGS = ['EN', 'ZH', 'JA', 'KO', 'AR', 'ES', 'PT', 'DE', 'FR', 'PL', 'RU', 'IT'];

export const LED_LEGEND = [
  { cls: 'c-blue', anim: 'blink', title: 'Blue blink', text: 'Transaction sending · awaiting confirmation' },
  { cls: 'c-green', anim: '', title: 'Green, 3 s', text: 'Confirmed' },
  { cls: 'c-red', anim: 'triple', title: 'Red triple-blink', text: 'Failed' },
  { cls: 'c-red', anim: 'sb', title: 'Red blink every 5 s', text: 'Battery under 10%' },
  { cls: 'c-yellow', anim: '', title: 'Yellow', text: 'Charging' },
  { cls: 'c-green', anim: '', title: 'Green', text: 'Full' },
  { cls: 'c-purple', anim: 'slow', title: 'Purple slow blink', text: "An AI agent's proposal is waiting for your press" },
];

export const PREORDER_CTA = 'Pre-order — $99 · refundable';
export const PREORDER_LINE = `Pre-order — $${PRICE_USD}. Ships ${SHIP_DATE}. Full refund any time before your unit ships, no questions.`;

export const GITHUB_URL = process.env.NEXT_PUBLIC_GITHUB_URL || '';
export const X_URL = process.env.NEXT_PUBLIC_X_URL || '';
