import type { Metadata } from 'next';
import Link from 'next/link';
import { PageHero } from '@/components/ui';
import { SHIP_DATE } from '@/lib/site';

export const metadata: Metadata = {
  title: 'Terms',
  description: 'Degen Walkie terms draft: we never hold your funds, trade-only keys, room service rules, no financial advice, perpetuals eligibility, pre-orders and refunds, firmware updates.',
};

export default function TermsPage() {
  return (
    <>
      <PageHero eyebrow="Terms" title={<>Plain-English<br /><span className="neon">terms.</span></>} lead="What you are agreeing to when you pre-order and use a Degen Walkie and the room service. Draft wording." />
      <section className="sec" aria-label="Terms draft">
        <div className="wrap legal">
          <p className="draft">Draft — not yet reviewed by counsel</p>
          <p>These draft terms cover the Degen Walkie device, its firmware, the Degen Walkie room service and pre-orders placed on this site. They will be replaced by reviewed terms before Genesis ships.</p>

          <h2>1. We never hold your funds</h2>
          <p>Degen Walkie is a trading terminal, not a wallet or a custodian. Your funds stay in your own accounts on third-party venues. The device stores trading keys you create and approve on those venues (for example a Hyperliquid agent key or a GMGN API key) in encrypted flash. We never hold your funds or those keys and cannot move, recover or freeze anything. You can revoke a key on the venue at any time, including if the device is lost. Funds you keep with a venue that holds them on your behalf, such as GMGN, are subject to that venue&apos;s terms, not ours.</p>

          <h2>2. Not financial advice</h2>
          <p>Rooms, prices, AI answers and anything said by other users or by AI agents are information, not advice. Nothing we show ranks, scores or endorses an asset. Voice orders only fill in a ticket; you decide whether to send it. You are responsible for every order you confirm.</p>

          <h2>3. Perpetuals and jurisdiction</h2>
          <p>Perpetual futures are available only where legal. Perpetuals are off by default. On first boot you confirm your jurisdiction; perpetual trading is enabled only in eligible regions. You are responsible for the accuracy of that confirmation and for complying with the laws that apply to you.</p>

          <h2>4. Third-party networks and venues</h2>
          <p>The device connects to independent blockchains and venues (for example Hyperliquid, Pacifica, Lighter, GMGN, Solana and Robinhood Chain). We do not operate them, are not responsible for their availability, fees, slippage, outages or losses, and their names are the property of their owners. Connections may change in firmware updates.</p>

          <h2>5. The room service</h2>
          <ul>
            <li>Room audio is processed by our servers for speech recognition, translation and synthesis, as described in the <Link href="/privacy">privacy draft</Link>.</li>
            <li>You may not use rooms to harass, defraud, impersonate, or to transmit unlawful content. Anti-spam rules are enforced automatically and may mute or restrict a device.</li>
            <li>We may suspend the room service for maintenance or abuse. Trading does not depend on the room service.</li>
          </ul>

          <h2>6. Firmware</h2>
          <p>The device runs only firmware signed by us and does not roll back to earlier versions. Updates may add, change or remove features, venues and regions. Some features described on this site are scheduled for firmware updates after Genesis ships and may change before release.</p>

          <h2>7. AI agents (MCP)</h2>
          <p>If you connect an agent through the MCP interface, you are responsible for that agent and what you allow it to read. Agents can only propose; every transaction requires a physical press on your device. We are not responsible for proposals an agent makes.</p>

          <h2>8. Pre-orders and refunds</h2>
          <ul>
            <li>A pre-order is a purchase of one Degen Walkie Genesis Edition at $99, paid in full through Stripe Checkout, limited to one unit per order and 1,000 units in total.</li>
            <li>Planned shipment is {SHIP_DATE} and depends on certification. We will tell pre-order holders if that date moves.</li>
            <li>You may cancel for a full refund at any time before your unit ships, for any reason, by emailing us. Refunds go to the original payment method. After shipment, the <Link href="/refund-policy">refund policy</Link> applies (30 days unopened).</li>
            <li>If we cannot ship to your region when your unit is ready, or if we cancel the Genesis run, we refund you in full.</li>
            <li>Prices, specifications and regions may change for future batches; your pre-order price does not.</li>
          </ul>

          <h2>9. Hardware</h2>
          <p>Genesis Edition shells are 3D-printed PA12 nylon and may show small cosmetic marks. Battery, weight and talk-time figures are targets until certification units are measured. Devices ship only to regions where certification is complete. Warranty terms will be published with the reviewed terms.</p>

          <h2>10. Limitation</h2>
          <p>To the extent permitted by law, we are not liable for losses arising from orders you confirm, from third-party networks and venues, from loss of your device before you revoke its keys, or from your use of information heard in rooms. Nothing here limits rights you have under consumer law in your region.</p>
        </div>
      </section>
    </>
  );
}
