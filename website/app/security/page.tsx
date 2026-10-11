import type { Metadata } from 'next';
import { PowerDemo, Shot } from '@/components/screens';
import { CheckBox, PageHero, PreorderButtons } from '@/components/ui';

export const metadata: Metadata = {
  title: 'Security',
  description: 'Degen Walkie security: we never hold your money. The device holds revocable trading keys; on Hyperliquid they cannot withdraw by protocol. Password lock, secure boot, encrypted flash, signed firmware with no downgrade, open-source firmware and schematic.',
};

const CHECKS: [string, string][] = [
  ['We never hold your money', 'Funds stay in your account on the venue. There is no Degen Walkie wallet, no deposit address and no server of ours that can move anything.'],
  ['Trade-only keys', 'On Hyperliquid the device holds an agent key your main wallet approved. It can place and cancel orders; the protocol does not let it withdraw or change its own permissions.'],
  ['Revoke any time', 'One press on the device, or from your main wallet or the venue without the device at all. Revoking cuts the key off immediately.'],
  ['Lost it? Revoke it.', 'A stolen device can at worst trade, and only until you revoke; on Hyperliquid it can never withdraw. Six wrong passwords also wipe the keys, with a warning before the last try.'],
  ['Venues vetted for withdrawal limits', 'A venue is connected only after we confirm the key we hold cannot move money out. Venues where that is still unconfirmed stay marked planned.'],
  ['Signed firmware, no downgrade', 'Only firmware we have signed runs, and it refuses to roll back. USB-C is for charging and signed updates. No debug port.'],
  ['Secure boot + encrypted flash', 'ESP32-S3 secure boot, flash encryption and eFuse keys burned at the factory. Trading keys live in encrypted flash behind your password.'],
  ['AI proposes, you confirm', 'MCP agents can read and queue proposals. Nothing is sent until the trade key is pressed on the device.'],
];

export default function SecurityPage() {
  return (
    <>
      <PageHero eyebrow="Security" title={<>We never hold<br />your <span className="neon">money.</span></>} lead="It stays in your account on the venue. The device only gets trading keys you approved and can revoke; on Hyperliquid the protocol itself won't let that key withdraw. Every order still needs a physical press on the thing in your hand." ctas={<PreorderButtons secondary="Open-source plan" secondaryHref="/roadmap#open" />} />

      <section className="sec" aria-labelledby="check-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Checklist</p><h2 id="check-title" className="display">Why you can relax.</h2></div>
          <div className="checklist rv">
            {CHECKS.map(([t, d]) => (
              <div className="check" key={t}><CheckBox /><div><h3>{t}</h3><p>{d}</p></div></div>
            ))}
          </div>
        </div>
      </section>

      <section className="sec alt" aria-labelledby="model-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Key model</p><h2 id="model-title" className="display">Main wallet → device. One way.</h2><p>Your main wallet stays wherever it lives today: browser, phone, hardware wallet. It approves a trading key for Degen Walkie once, on the venue. The device never sees your main wallet&apos;s seed.</p></div>
          <div className="wallets rv">
            <div className="wallet trading"><div className="idx">Your main wallet</div><h3>Owns</h3><ul><li>Holds the account on the venue</li><li>The only thing that can withdraw</li><li>Approves and revokes the device</li></ul></div>
            <div className="arrow"><b>→</b>approve once<br />revoke any time</div>
            <div className="wallet vault"><div className="idx">Degen Walkie</div><h3>Trades</h3><ul><li>Places and cancels orders</li><li>Cannot withdraw</li><li>Cannot change its own permissions</li></ul></div>
          </div>
          <div className="screens rv d1 mt-36"><Shot id="accounts" /><Shot id="link" /><Shot id="revoke" /></div>
          <p className="fine mt-28">Meme coins run through your GMGN account and its trading API. Those funds are held by GMGN under GMGN&apos;s terms, not by us; the device holds a GMGN API key you can revoke in GMGN.</p>
        </div>
      </section>

      <section className="sec" aria-labelledby="agent-title">
        <div className="wrap">
          <div className="split v-center">
            <div className="rv">
              <p className="eyebrow hot">MCP · AI agents</p>
              <h2 id="agent-title" className="display">AI can propose.<br />Only your thumb can <span className="neon">confirm.</span></h2>
              <p className="lead mt-18">An agent can read positions and room context and queue a proposal. The purple light blinks. The proposal shows as a confirmation sheet with every detail. The trade key confirms; the wheel press dismisses. There is no third option, and no API that skips the key.</p>
            </div>
            <div className="rv d1"><Shot id="agent" /></div>
          </div>
        </div>
      </section>

      <section className="sec alt" aria-labelledby="pwr-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Power key</p><h2 id="pwr-title" className="display">Off means <span className="neon">touched.</span></h2><p>Short press turns the screen off while the room keeps playing. Holding two seconds opens the power-off sheet, and only a finger sliding across the glass confirms it. No key, no agent, no accidental pocket press can power it down.</p></div>
          <div className="rv"><PowerDemo /></div>
        </div>
      </section>

      <section className="sec" id="open" aria-labelledby="open-title">
        <div className="wrap">
          <div className="split v-center">
            <div className="rv">
              <p className="eyebrow">Open source</p>
              <h2 id="open-title" className="display">Audit it<br />yourself.</h2>
              <p className="lead mt-18">Firmware and schematic go public during the pre-order, before Genesis ships, so you can read the code that holds your trading keys before your unit arrives. The MCP interface spec is published with them.</p>
            </div>
            <div className="grid g2 rv d1">
              <div className="card neonline"><div className="idx">Open · pre-order</div><h3>Firmware</h3><p>Key storage, order signing, secure boot. The code that touches your keys.</p></div>
              <div className="card neonline"><div className="idx">Open · pre-order</div><h3>Schematic</h3><p>Know exactly what&apos;s on the board.</p></div>
              <div className="card neonline"><div className="idx">Open · pre-order</div><h3>MCP spec</h3><p>So any agent framework can propose.</p></div>
              <div className="card"><div className="idx">Open when Genesis ships</div><h3>Enclosure CAD</h3><p>Print your own shell. Mod it.</p></div>
            </div>
          </div>
        </div>
      </section>
    </>
  );
}
