// Small crypto and encoding helpers shared by the mocks and the phone.
import { keccak_256 } from '@noble/hashes/sha3';
import { secp256k1 } from '@noble/curves/secp256k1';
import { ed25519 } from '@noble/curves/ed25519';

export const te = new TextEncoder();
export const td = new TextDecoder();

export const hex = (b) => Array.from(b, (x) => x.toString(16).padStart(2, '0')).join('');
export const unhex = (s) => {
  s = s.startsWith('0x') ? s.slice(2) : s;
  const out = new Uint8Array(s.length / 2);
  for (let i = 0; i < out.length; i++) out[i] = parseInt(s.substr(i * 2, 2), 16);
  return out;
};
export const concat = (...parts) => {
  const n = parts.reduce((a, p) => a + p.length, 0);
  const out = new Uint8Array(n);
  let o = 0;
  for (const p of parts) { out.set(p, o); o += p.length; }
  return out;
};
export const keccak = (b) => keccak_256(typeof b === 'string' ? te.encode(b) : b);
export const randomBytes = (n) => crypto.getRandomValues(new Uint8Array(n));

const B58 = '123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz';
export function b58encode(bytes) {
  let n = 0n;
  for (const b of bytes) n = n * 256n + BigInt(b);
  let s = '';
  while (n > 0n) { s = B58[Number(n % 58n)] + s; n /= 58n; }
  for (const b of bytes) { if (b !== 0) break; s = '1' + s; }
  return s;
}
export function b58decode(s) {
  let n = 0n;
  for (const c of s) {
    const i = B58.indexOf(c);
    if (i < 0) throw new Error('bad base58');
    n = n * 58n + BigInt(i);
  }
  const out = [];
  while (n > 0n) { out.unshift(Number(n % 256n)); n /= 256n; }
  for (const c of s) { if (c !== '1') break; out.unshift(0); }
  return new Uint8Array(out);
}
export const b64decode = (s) => Uint8Array.from(atob(s), (c) => c.charCodeAt(0));

/* ---- Ethereum ---- */
export function ethAddress(pubUncompressed) {
  return '0x' + hex(keccak(pubUncompressed.slice(1)).slice(12));
}
export function checksum(addr) {
  const a = addr.toLowerCase().replace(/^0x/, '');
  const h = hex(keccak(a));
  return '0x' + [...a].map((c, i) => (parseInt(h[i], 16) >= 8 ? c.toUpperCase() : c)).join('');
}
export function addressOfPriv(priv) {
  return checksum(ethAddress(secp256k1.getPublicKey(priv, false)));
}

// EIP-712 for the handful of flat types Hyperliquid uses.
const word = (v) => { const out = new Uint8Array(32); out.set(v, 32 - v.length); return out; };
const uint = (n) => { let x = BigInt(n); const out = new Uint8Array(32); for (let i = 31; i >= 0; i--) { out[i] = Number(x & 255n); x >>= 8n; } return out; };
function encodeValue(type, v) {
  if (type === 'string') return keccak(v);
  if (type === 'address') return word(unhex(v));
  if (type === 'bytes32') return typeof v === 'string' ? unhex(v) : v;
  if (type.startsWith('uint')) return uint(v);
  if (type === 'bool') return uint(v ? 1 : 0);
  throw new Error('eip712 type ' + type);
}
export function hashStruct(name, fields, values) {
  const typeStr = `${name}(${fields.map(([t, n]) => `${t} ${n}`).join(',')})`;
  return keccak(concat(keccak(typeStr), ...fields.map(([t, n]) => encodeValue(t, values[n]))));
}
export function eip712Digest(domain, primary, fields, values) {
  const dom = hashStruct('EIP712Domain', [['string', 'name'], ['string', 'version'], ['uint256', 'chainId'], ['address', 'verifyingContract']], domain);
  return keccak(concat(new Uint8Array([0x19, 0x01]), dom, hashStruct(primary, fields, values)));
}
export function signDigest(priv, digest) {
  const s = secp256k1.sign(digest, priv);
  return { r: '0x' + s.r.toString(16).padStart(64, '0'), s: '0x' + s.s.toString(16).padStart(64, '0'), v: 27 + s.recovery };
}
export function recoverAddress(digest, sig) {
  const s = new secp256k1.Signature(BigInt(sig.r), BigInt(sig.s)).addRecoveryBit(Number(sig.v) - 27);
  return checksum(ethAddress(s.recoverPublicKey(digest).toRawBytes(false)));
}

/* ---- Ed25519 ---- */
export const edPub = (seed) => ed25519.getPublicKey(seed);
export const edSign = (msg, seed) => ed25519.sign(typeof msg === 'string' ? te.encode(msg) : msg, seed);
export const edVerify = (sig, msg, pub) => {
  try { return ed25519.verify(sig, typeof msg === 'string' ? te.encode(msg) : msg, pub); } catch { return false; }
};

/* JSON with keys sorted recursively and no spaces (Pacifica's signing format). */
export function sortedJson(v) {
  if (Array.isArray(v)) return '[' + v.map(sortedJson).join(',') + ']';
  if (v && typeof v === 'object') return '{' + Object.keys(v).sort().map((k) => JSON.stringify(k) + ':' + sortedJson(v[k])).join(',') + '}';
  return JSON.stringify(v);
}

export const short = (s, a = 6, b = 4) => (s && s.length > a + b + 1 ? `${s.slice(0, a)}…${s.slice(-b)}` : s);
export const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
