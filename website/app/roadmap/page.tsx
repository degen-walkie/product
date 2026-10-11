import type { Metadata } from 'next';
import Link from 'next/link';
import { PageHero, PreorderButtons } from '@/components/ui';
import { CERT_LINE, SHIP_DATE } from '@/lib/site';

export const metadata: Metadata = {
  title: 'Roadmap',
  description: 'Degen Walkie roadmap: Genesis pre-orders, certification, shipping January 2027, early open-source firmware and schematic, more trading venues, MCP agent proposals and the 4G back-clip.',
};

export default function RoadmapPage() {
  return (
    <>
      <PageHero eyebrow="Roadmap" title={<>Ships. Then<br />keeps <span className="neon">shipping.</span></>} lead="Genesis is the first batch, not the last word. The hardware is fixed; the firmware is signed and updates over the air. Here is the order things land in." ctas={<PreorderButtons />} />
      <section className="sec" aria-labelledby="road-title">
        <div className="wrap">
          <h2 id="road-title" className="sr">Timeline</h2>
          <ol className="road">
            <li className="now rv"><div className="when">Now</div><h3>Genesis pre-orders open</h3><p>1,000 units at $99, paid via Stripe, full refund any time before your unit ships. First wave ships to the United States, Canada, the European Union, the United Kingdom, Japan, Hong Kong and Singapore — more regions as approvals land.</p></li>
            <li className="rv" id="open"><div className="when">During pre-order</div><h3>Open source, early</h3><p>Firmware and schematic go public before Genesis ships, so you can read the code that holds your trading keys before your unit arrives. The MCP interface spec is published with them. Enclosure CAD follows at ship. The PCB layout stays closed for now.</p>
              <ul><span className="tag neon">Firmware</span><span className="tag neon">Schematic</span><span className="tag neon">MCP spec</span></ul></li>
            <li className="rv"><div className="when">In progress</div><h3>Certification</h3><p>{CERT_LINE}. Genesis ships after certification, region by region.</p></li>
            <li className="rv"><div className="when">{SHIP_DATE}</div><h3>Genesis ships · launch firmware</h3><p>Perps on Hyperliquid, Pacifica (Solana) and Lighter (Robinhood Chain) where eligible, through trading keys you can revoke. Live rooms by ticker with 12-language translation, world room and radio mode. Touch-first order tickets, Ask the AI and voice shortcuts. Spot on Hyperliquid and memes through GMGN on the side. QR Wi-Fi setup and account linking, CJK keyboards. MCP read access and the proposal queue.</p>
              <ul><span className="tag neon">Perps</span><span className="tag neon">Live rooms</span><span className="tag neon">Ask AI</span><span className="tag neon">Voice shortcuts</span><span className="tag neon">MCP read + propose</span></ul></li>
            <li className="rv" id="mcp"><div className="when">Firmware updates</div><h3>AI can propose. Only your thumb can confirm.</h3><p>MCP access deepens in firmware updates: richer room context for agents, proposal templates, multi-step proposals that still need one press per order. The purple light means your agent wants something; the trade key is the only yes.</p>
              <ul><span className="tag hot">MCP</span><span className="tag hot">Agents</span></ul></li>
            <li className="soon rv"><div className="when">Coming soon</div><h3>4G back-clip</h3><p>Magnetic, snaps onto the six gold contacts, cellular with its own battery and SIM. Firmware support lands first; the clip follows. Accessory, sold separately. <Link href="/clip">Details</Link>.</p></li>
            <li className="rv"><div className="when">Ongoing</div><h3>More regions, more languages</h3><p>Each as approvals and integrations land, in firmware updates. Nothing you own gets left behind: Genesis hardware runs every update.</p></li>
          </ol>
        </div>
      </section>
    </>
  );
}
