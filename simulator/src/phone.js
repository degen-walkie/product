// The user's phone and main wallets. In real life this is MetaMask / Phantom / the
// venue's own site; here it is a panel that signs the same approval messages.
import { secp256k1 } from '@noble/curves/secp256k1';
import * as L from './ledger.js';
import { http } from './net.js';
import { completeLink } from './mock_relay.js';
import { userDigest } from './mock_hyperliquid.js';
import { BASE as HL } from './mock_hyperliquid.js';
import { BASE as PAC } from './mock_pacifica.js';
import { BASE as LIT } from './mock_lighter.js';
import { addressOfPriv, b58encode, edPub, edSign, hex, randomBytes, signDigest, sortedJson, unhex } from './util.js';

const KEY = 'dw.phone';
const LIT_ACCOUNT = 4821;
const LIT_KEY_INDEX = 3;

function loadWallets() {
  try {
    const w = JSON.parse(localStorage.getItem(KEY));
    if (w && w.hl && w.pac) return w;
  } catch { /* new */ }
  const w = { hl: hex(secp256k1.utils.randomPrivateKey()), pac: hex(randomBytes(32)) };
  localStorage.setItem(KEY, JSON.stringify(w));
  return w;
}

const W = loadWallets();
export const wallets = {
  hl: { priv: unhex(W.hl), address: addressOfPriv(unhex(W.hl)) },
  pac: { seed: unhex(W.pac), pubkey: b58encode(edPub(unhex(W.pac))) },
  lit: { index: LIT_ACCOUNT, keyIndex: LIT_KEY_INDEX },
};
wallets.lit.l1 = wallets.hl.address;

function ensureFunded(venue, id) {
  const fresh = !L.db[venue].accounts[id] || !L.db[venue].accounts[id].seeded;
  const a = L.account(venue, id);
  if (fresh) { L.seedDemo(venue, a); a.seeded = true; L.save(); }
}

/* ---- approvals (what "Approve" in the main wallet does) ---- */

export async function approveHyperliquid(agentAddress, tag = 'phone') {
  const action = { type: 'approveAgent', signatureChainId: '0x66eee', hyperliquidChain: 'Mainnet', agentAddress, agentName: 'walkie', nonce: Date.now() };
  const signature = signDigest(wallets.hl.priv, userDigest(action));
  ensureFunded('hl', wallets.hl.address.toLowerCase());
  const r = await http('POST', `${HL}/exchange`, 'X-From: ' + tag, JSON.stringify({ action, nonce: action.nonce, signature }));
  return JSON.parse(r.body).status === 'ok';
}

export async function approvePacifica(agentPub58, tag = 'phone') {
  const timestamp = Date.now(), expiry_window = 5000;
  const msg = sortedJson({ type: 'bind_agent_wallet', timestamp, expiry_window, data: { agent_wallet: agentPub58 } });
  const signature = b58encode(edSign(msg, wallets.pac.seed));
  ensureFunded('pac', wallets.pac.pubkey);
  const r = await http('POST', `${PAC}/agent/bind`, 'X-From: ' + tag, JSON.stringify({ account: wallets.pac.pubkey, signature, timestamp, expiry_window, agent_wallet: agentPub58 }));
  return JSON.parse(r.body).success;
}

export async function approveLighter(pubHex, tag = 'phone') {
  const tx = { AccountIndex: LIT_ACCOUNT, ApiKeyIndex: LIT_KEY_INDEX, PubKey: pubHex, L1Address: wallets.lit.l1, Nonce: 0, ExpiredAt: Date.now() + 600e3, L1Sig: '0x' + hex(randomBytes(65)) };
  ensureFunded('lit', String(LIT_ACCOUNT));
  const body = `tx_type=8&tx_info=${encodeURIComponent(JSON.stringify(tx))}`;
  const r = await http('POST', `${LIT}/sendTx`, 'Content-Type: application/x-www-form-urlencoded\nX-From: ' + tag, body);
  return JSON.parse(r.body).code === 200;
}

/* The QR on the device: https://degenwalkie.trade/link?v=hl&agent=0x..&s=session */
export function parseQr(qr) {
  try {
    const u = new URL(qr);
    const p = u.searchParams;
    return { venue: p.get('v'), agent: p.get('agent'), pubkey: p.get('pubkey'), session: p.get('s') };
  } catch { return null; }
}

export async function approveQr(qr) {
  const q = parseQr(qr);
  if (!q) return { ok: false, msg: 'Not a Degen Walkie link' };
  let ok = false;
  if (q.venue === 'hl') ok = await approveHyperliquid(q.agent);
  if (q.venue === 'pac') ok = await approvePacifica(q.agent);
  if (q.venue === 'lit') ok = await approveLighter(q.pubkey);
  if (!ok) return { ok: false, msg: 'The venue rejected the approval' };
  const account = q.venue === 'hl' ? wallets.hl.address : q.venue === 'pac' ? wallets.pac.pubkey : String(LIT_ACCOUNT);
  completeLink(q.session, q.venue, account, q.venue === 'lit' ? String(LIT_KEY_INDEX) : '');
  return { ok: true, venue: q.venue };
}

/* ---- the attack demo: take the device's own key and try to withdraw ---- */
export async function tryWithdrawWithDeviceKey() {
  const k = localStorage.getItem('dw.fw.demo.hl.key');
  if (!k) return { ok: false, text: 'The device has no Hyperliquid key yet. Link Hyperliquid first.' };
  const action = { type: 'withdraw3', signatureChainId: '0x66eee', hyperliquidChain: 'Mainnet', destination: '0x000000000000000000000000000000000000dEaD', amount: '1000', time: Date.now() };
  const signature = signDigest(unhex(k), userDigest(action));
  const r = await http('POST', `${HL}/exchange`, 'X-From: attacker', JSON.stringify({ action, nonce: action.time, signature }));
  return { ok: true, text: r.body };
}

/* ---- first run: a demo device with two venues already linked ---- */
export async function provisionDemo() {
  if (localStorage.getItem('dw.demo.provisioned')) return false;
  const hlKey = secp256k1.utils.randomPrivateKey();
  const pacSeed = randomBytes(32);
  localStorage.setItem('dw.fw.demo.hl.key', hex(hlKey));
  localStorage.setItem('dw.fw.demo.hl.acct', wallets.hl.address);
  localStorage.setItem('dw.fw.demo.pac.key', hex(pacSeed));
  localStorage.setItem('dw.fw.demo.pac.acct', wallets.pac.pubkey);
  await approveHyperliquid(addressOfPriv(hlKey), 'setup');
  await approvePacifica(b58encode(edPub(pacSeed)), 'setup');
  localStorage.setItem('dw.demo.provisioned', '1');
  return true;
}

export function resetAll() {
  for (const k of Object.keys(localStorage)) if (k.startsWith('dw.')) localStorage.removeItem(k);
}
