// The page a phone opens after scanning the device's QR. It asks the user's own wallet
// to approve the device key, then tells the device through the relay.
import * as W from './wallet.js';

const $ = (id) => document.getElementById(id);
const link = W.parseLink(location.href);
const relay = (link && link.relay) || (/^(localhost|127\.)/.test(location.hostname) ? 'http://localhost:8787/v1' : 'https://relay-demo.degenwalkie.trade/v1');

if (!link || !link.agent || !link.session) {
  $('detail').innerHTML = '<p class="muted">This link is incomplete. Scan the QR on the device again.</p>';
  $('go').disabled = true;
} else {
  const venue = link.venue === 'hl' ? 'Hyperliquid' : link.venue === 'pac' ? 'Pacifica' : 'Lighter';
  $('net').textContent = link.net;
  $('net').className = `net ${link.net}`;
  $('detail').innerHTML = `<div class="approve"><div class="eyebrow">${venue} · approval request</div>
    <dl><dt>Device key</dt><dd>${W.short(link.agent, 10, 6)}</dd><dt>Can</dt><dd class="ok">place, change and cancel orders</dd>
    <dt>Cannot</dt><dd class="no">withdraw or transfer your funds</dd></dl></div>`;
  $('hint').textContent = link.venue === 'hl'
    ? 'Open this page in your EVM wallet\'s browser (MetaMask, Rabby, Coinbase Wallet) if the button does nothing.'
    : 'Open this page in your Solana wallet\'s browser (Phantom, Backpack) if the button does nothing.';
  $('go').addEventListener('click', async () => {
    const log = (t) => { $('log').hidden = false; $('log').textContent += t + '\n'; };
    $('go').disabled = true;
    try {
      const acct = await W.approveLink(link, relay, log);
      log(`Done. Look at the device: it is linked to ${W.short(acct)}.`);
    } catch (e) {
      log(`Stopped: ${e.message || e}`);
      $('go').disabled = false;
    }
  });
}
