import type { Metadata } from 'next';
import { MenuDemo, NavDemo, PowerDemo, Shot, SpotDemo, type ScreenId } from '@/components/screens';
import { Glyph } from '@/components/screens/parts';
import { LedLegend, PageHero, PreorderButtons } from '@/components/ui';

export const metadata: Metadata = {
  title: 'Screens',
  description: 'The Degen Walkie on-device UI and its three controls: voice order, ask the AI, side menu, wheel navigation, group-chat world room, spot order with confirmation sheet, perps, meme ticket, linked accounts, linking and revoking trade-only keys, languages, AI proposal, charging, power off.',
};

const ORDER: ScreenId[] = ['rooms', 'room', 'world', 'perps', 'asset', 'ask', 'voice', 'spot', 'accounts', 'link', 'revoke', 'agent', 'meme', 'settings', 'charging'];

export default function ScreensPage() {
  return (
    <>
      <PageHero eyebrow="Screens · on-device UI" title={<>320 × 480.<br />Big <span className="neon">thumbs.</span></>} lead="Dark, mono numbers, touch targets you can hit while watching a chart. Every screen has exactly one focus — the neon outline — and a three-slot hint bar at the bottom that says what roll, press and the trade key do right now." ctas={<PreorderButtons secondary="The hardware" secondaryHref="/hardware" />} />

      <section className="sec" aria-labelledby="rules-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Controls</p><h2 id="rules-title" className="display">Roll. Back. Open.</h2><p>Three physical inputs, three meanings, no long-presses. Everything the keys do, touch can do too; the keys are just faster.</p></div>
          <div className="rules rv">
            <div className="rule">
              <Glyph kind="wheel" state="down" />
              <h3>Wheel roll = move</h3>
              <ul>
                <li><b>lists</b><span>moves the focus through rows and fields</span></li>
                <li><b>numbers</b><span>adjusts the value when the focus is on an amount, slippage or leverage</span></li>
                <li><b>pages</b><span>flips prev / next in paged views: room Chat | Chart | Orders, asset Chart | Trade | Holders</span></li>
              </ul>
            </div>
            <div className="rule">
              <Glyph kind="wheel" state="press" />
              <h3>Wheel press = back</h3>
              <ul>
                <li><b>sheets</b><span>closes a sheet or dialog</span></li>
                <li><b>levels</b><span>goes up one level</span></li>
                <li><b>top level</b><span>nothing to go back to, so it toggles the side menu. The menu is the special back; closing it is a back too.</span></li>
              </ul>
            </div>
            <div className="rule">
              <Glyph kind="trade" state="press" />
              <h3>Trade key = open</h3>
              <ul>
                <li><b>focus</b><span>opens the focused item; in the side menu, the highlighted section</span></li>
                <li><b>numbers</b><span>confirms the value</span></li>
                <li><b>rooms</b><span>the default focus is the ticker header, so it opens that ticket. The mic stays on-screen.</span></li>
                <li><b>sheets</b><span>on a confirmation sheet it signs. Signing is the special enter.</span></li>
              </ul>
            </div>
          </div>
          <p className="fine">Power key: short press turns the screen off or on while the room keeps playing; hold two seconds for the slide-to-power-off sheet. Only a touch slide confirms.</p>
        </div>
      </section>

      <section className="sec alt" aria-labelledby="motion-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">In motion</p><h2 id="motion-title" className="display">Watch the wheel work.</h2><p>Looping demos. The glyph beside each device shows which control is moving.</p></div>
          <div className="grid" style={{ gap: 48 }}>
            <div className="rv"><NavDemo /></div>
            <div className="rv"><MenuDemo /></div>
            <div className="rv"><SpotDemo /></div>
            <div className="rv"><PowerDemo /></div>
          </div>
        </div>
      </section>

      <section className="sec" aria-labelledby="ui-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">System</p><h2 id="ui-title" className="display">One language of screens.</h2><p>Every screen shares the same bones: status bar on top, one focus, the three-slot hint bar at the bottom, and the trade key for anything that places an order. Rendered here in HTML; the device runs the same layout at 320 × 480.</p></div>
          <div className="screens four">
            {ORDER.map((k, i) => (
              <Shot key={k} id={k} delay={i % 4 ? `d${i % 4}` : ''} animTag={k === 'world' ? 'animated' : k === 'room' ? 'live mic' : undefined} />
            ))}
          </div>
          <p className="fine mt-28">Illustrative screens. Prices, handles, addresses and balances are placeholders. Final firmware may differ.</p>
        </div>
      </section>

      <section className="sec alt" aria-labelledby="led-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Status light</p><h2 id="led-title" className="display">Read it from across the table.</h2></div>
          <div className="rv"><LedLegend wide /></div>
        </div>
      </section>
    </>
  );
}
