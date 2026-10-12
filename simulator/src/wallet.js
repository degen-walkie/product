// Real main wallets, for real mode and the phone link page:
//   Hyperliquid: an EVM wallet (MetaMask, Rabby…) signs approveAgent with EIP-712.
//   Pacifica:    a Solana wallet (Phantom, Backpack…) signs bind_agent_wallet.
// The device's key is the one in the QR. Nothing here ever sees a private key.
import { b58encode, sortedJson } from './util.js';

export const ENDPOINTS = {
  TESTNET: { hl: 'https://api.hyperliquid-testnet.xyz', pac: 'https://test-api.pacifica.fi/api/v1', hlApp: 'https://app.hyperliquid-testnet.xyz', pacApp: 'https://test-app.pacifica.fi' },
  MAINNET: { hl: 'https://api.hyperliquid.xyz', pac: 'https://api.pacifica.fi/api/v1', hlApp: 'https://app.hyperliquid.xyz', pacApp: 'https://app.pacifica.fi' },
};

/* https://degenwalkie.trade/link?v=hl&agent=0x..&s=<session>&n=TESTNET&r=<relay> */
export function parseLink(text) {
  try {
    const u = new URL(text);
    const p = u.searchParams;
    return { venue: p.get('v'), agent: p.get('agent') || p.get('pubkey'), session: p.get('s'), net: (p.get('n') || 'MAINNET').toUpperCase(), relay: p.get('r') };
  } catch { return null; }
}

/* ---------------- EVM (Hyperliquid) ---------------- */

export function evmProvider() { return window.ethereum || null; }

export async function connectEvm() {
  const eth = evmProvider();
  if (!eth) throw new Error('No EVM wallet found. Install MetaMask or Rabby.');
  const [address] = await eth.request({ method: 'eth_requestAccounts' });
  const chainId = await eth.request({ method: 'eth_chainId' });
  return { address, chainId };
}

export async function approveHyperliquid(agentAddress, net, log = () => {}) {
  const eth = evmProvider();
  const { address, chainId } = await connectEvm();
  const nonce = Date.now();
  const hyperliquidChain = net === 'MAINNET' ? 'Mainnet' : 'Testnet';
  const action = { type: 'approveAgent', signatureChainId: chainId, hyperliquidChain, agentAddress, agentName: 'walkie', nonce };
  const typed = {
    domain: { name: 'HyperliquidSignTransaction', version: '1', chainId: parseInt(chainId, 16), verifyingContract: '0x0000000000000000000000000000000000000000' },
    types: {
      EIP712Domain: [{ name: 'name', type: 'string' }, { name: 'version', type: 'string' }, { name: 'chainId', type: 'uint256' }, { name: 'verifyingContract', type: 'address' }],
      'HyperliquidTransaction:ApproveAgent': [{ name: 'hyperliquidChain', type: 'string' }, { name: 'agentAddress', type: 'address' }, { name: 'agentName', type: 'string' }, { name: 'nonce', type: 'uint64' }],
    },
    primaryType: 'HyperliquidTransaction:ApproveAgent',
    message: { hyperliquidChain, agentAddress, agentName: 'walkie', nonce },
  };
  log(`Asking ${short(address)} to approve agent ${short(agentAddress)} on Hyperliquid ${hyperliquidChain}…`);
  const sig = await eth.request({ method: 'eth_signTypedData_v4', params: [address, JSON.stringify(typed)] });
  const signature = { r: '0x' + sig.slice(2, 66), s: '0x' + sig.slice(66, 130), v: parseInt(sig.slice(130, 132), 16) };
  const r = await fetch(`${ENDPOINTS[net].hl}/exchange`, {
    method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ action, nonce, signature }),
  });
  const body = await r.json().catch(() => ({}));
  if (body.status !== 'ok') throw new Error(typeof body.response === 'string' ? body.response : `Hyperliquid said ${r.status}`);
  log('Hyperliquid accepted the agent. It can trade; it cannot withdraw.');
  return address;
}

/* ---------------- Solana (Pacifica) ---------------- */

export function solProvider() { return (window.phantom && window.phantom.solana) || window.solana || window.backpack || null; }

export async function connectSol() {
  const sol = solProvider();
  if (!sol) throw new Error('No Solana wallet found. Install Phantom or Backpack.');
  const res = await sol.connect();
  return (res && res.publicKey ? res.publicKey : sol.publicKey).toString();
}

export async function approvePacifica(agentWallet, net, log = () => {}) {
  const sol = solProvider();
  const account = await connectSol();
  const timestamp = Date.now(), expiry_window = 5000;
  const msg = sortedJson({ type: 'bind_agent_wallet', timestamp, expiry_window, data: { agent_wallet: agentWallet } });
  log(`Asking ${short(account)} to bind agent ${short(agentWallet)} on Pacifica ${net.toLowerCase()}…`);
  const signed = await sol.signMessage(new TextEncoder().encode(msg), 'utf8');
  const sigBytes = signed.signature || signed;
  const r = await fetch(`${ENDPOINTS[net].pac}/agent/bind`, {
    method: 'POST', headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ account, signature: b58encode(sigBytes), timestamp, expiry_window, agent_wallet: agentWallet }),
  });
  const body = await r.json().catch(() => ({}));
  if (!body.success) throw new Error(body.error || `Pacifica said ${r.status}`);
  log('Pacifica bound the agent wallet to your account.');
  return account;
}

/* ---------------- tell the device (through the relay) ---------------- */

export async function completeLink(relay, session, venue, account) {
  const r = await fetch(`${relay}/link/${session}`, {
    method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ venue, account }),
  });
  if (!r.ok) throw new Error(`The relay said ${r.status}`);
}

export async function approveLink(link, relay, log) {
  if (link.venue === 'hl') {
    const account = await approveHyperliquid(link.agent, link.net, log);
    await completeLink(relay, link.session, 'hl', account);
    return account;
  }
  if (link.venue === 'pac') {
    const account = await approvePacifica(link.agent, link.net, log);
    await completeLink(relay, link.session, 'pac', account);
    return account;
  }
  throw new Error('Lighter is coming soon.');
}

/* ---------------- balances for the panel ---------------- */

export async function hlEquity(net, user) {
  const r = await fetch(`${ENDPOINTS[net].hl}/info`, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ type: 'clearinghouseState', user }) });
  const j = await r.json();
  return +j.marginSummary.accountValue;
}

export async function pacEquity(net, account) {
  const r = await fetch(`${ENDPOINTS[net].pac}/account?account=${account}`);
  const j = await r.json();
  return j.success ? +j.data.account_equity : null;
}

export const short = (s, a = 6, b = 4) => (s && s.length > a + b + 1 ? `${s.slice(0, a)}…${s.slice(-b)}` : s || '');
