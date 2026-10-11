import Link from 'next/link';
import Viewer from '@/components/Viewer';
import BuiltInOpen from '@/components/BuiltInOpen';
import { Shot } from '@/components/screens';
import { Poster, PreorderButtons, Regions, Tape } from '@/components/ui';
import { CERT_LINE, SHIP_DATE } from '@/lib/site';

export default function HomePage() {
  return (
    <>
      <section className="hero" aria-labelledby="hero-title">
        <div className="wrap">
          <div className="hero-copy">
            <p className="eyebrow">Genesis Edition · 1,000 units · $99</p>
            <h1 id="hero-title" className="display">
              <span className="glitch">Talk your book.</span>
              <br />
              Then <span className="neon">trade it.</span>
            </h1>
            <p className="lead">A perp trading terminal with a live room for every ticker. Hear traders, pros and an AI talk the market in your language. Build the order in a few taps, press the physical trade key to send. Hyperliquid, Pacifica and Lighter. We never hold your money.</p>
            <div className="price-row">
              <span className="price">$99</span>
              <span className="price-meta">Genesis Edition · PA12 nylon shell · 1,000 units · ships {SHIP_DATE}</span>
            </div>
            <div className="cta-row">
              <PreorderButtons secondary="See the hardware" secondaryHref="/hardware" />
            </div>
            <p className="fine">Full refund any time before your unit ships, no questions.</p>
            <div className="stats" aria-label="Key numbers">
              <div><div className="k">3</div><div className="l">perp DEXs</div></div>
              <div><div className="k">12</div><div className="l">languages, live</div></div>
              <div><div className="k">1</div><div className="l">key to trade</div></div>
              <div><div className="k">$0</div><div className="l">of yours we hold</div></div>
            </div>
          </div>
          <Viewer view="front" light="cycle" controls fit={1.1} />
        </div>
      </section>

      <Tape />

      <section className="sec" aria-labelledby="how-title">
        <div className="wrap">
          <div className="sec-head rv">
            <p className="eyebrow">How it works</p>
            <h2 id="how-title" className="display">Three moves.</h2>
          </div>
          <div className="steps">
            <div className="step rv"><div className="n">01</div><h3>Hear the room</h3><p>Every ticker is a live room: #SOL, #BTC, #HYPE. Traders, pros and an AI talk the market, and you hear it all in your language. Hold the mic to join in, or to ask the AI.</p></div>
            <div className="step rv d1"><div className="n">02</div><h3>Tap the ticket</h3><p>The room you&apos;re in is the market you trade. Side, market or limit, leverage, size presets, TP / SL: a few taps on the glass. The wheel fine-tunes any number.</p></div>
            <div className="step rv d2"><div className="n hot">03</div><h3>Press to send</h3><p>The trade key on the right edge sends it. Nothing goes out without that press, not from a tap, your voice or an AI. Blue light while it sends, green when it fills.</p></div>
          </div>
          <div className="screens rv d1 mt-36"><Shot id="room" /><Shot id="perps" /><Shot id="ask" /></div>
        </div>
      </section>

      <section className="sec alt" aria-labelledby="feat-title">
        <div className="wrap">
          <div className="sec-head rv">
            <p className="eyebrow">What&apos;s inside</p>
            <h2 id="feat-title" className="display">Built to trade.</h2>
          </div>
          <div className="grid g3">
            <Link className="card rv" href="/talk"><div className="idx">Live rooms</div><h3>One room per ticker.</h3><p>Hear what traders are saying about the market you trade, translated live into 12 languages. Follow the pros who host rooms. Hold the mic to talk back.</p><span className="more">Rooms →</span></Link>
            <Link className="card rv d1" href="/trade"><div className="idx">Perps</div><h3>Three perp DEXs. Touch-fast.</h3><p>Hyperliquid, Pacifica on Solana, Lighter on Robinhood Chain. Tickets built in a few taps, sent with one press. <small className="mute">Perps in eligible regions only.</small></p><span className="more">Trade →</span></Link>
            <Link className="card rv d2" href="/security"><div className="idx">Security</div><h3>We never hold your money.</h3><p>It stays on the venue. The device gets a revocable trading key; on Hyperliquid it can&apos;t withdraw, by protocol. Lose it? Revoke it.</p><span className="more">Security →</span></Link>
            <Link className="card rv hotline" href="/talk#ask"><div className="idx hot">AI in the room</div><h3>Ask it anything.</h3><p>&ldquo;How far is my SOL long from liquidation?&rdquo; Ask out loud and get the answer on screen. Agents can propose over MCP; only your thumb can confirm.</p><span className="more hot">Ask the AI →</span></Link>
            <Link className="card rv d1" href="/talk#voice"><div className="idx">Voice shortcuts</div><h3>Say it when your hands are busy.</h3><p>&ldquo;Close half my ETH.&rdquo; Voice fills in the ticket, you check it, the trade key sends it. Touch stays the fast path.</p><span className="more">Voice →</span></Link>
            <Link className="card rv d2" href="/hardware"><div className="idx">Hardware</div><h3>Glance, don&apos;t unlock.</h3><p>Always-on screen next to your keyboard, a status light you read from across the desk, a trade key you find blind.</p><span className="more">Hardware →</span></Link>
          </div>
        </div>
      </section>

      <section className="sec" aria-labelledby="cold-title">
        <div className="wrap">
          <div className="split v-center">
            <div className="rv">
              <p className="eyebrow">Not another cold wallet</p>
              <h2 id="cold-title" className="display">Keep yours.<br />It approves us <span className="neon">once.</span></h2>
              <p className="lead mt-18">Cold wallets are for holding. Degen Walkie is for trading. Your wallet, cold or not, approves a trading key for the device on the venue, then goes back in the drawer. The device trades; it never holds your money, and you can revoke it from your wallet at any time.</p>
              <div className="cta-row"><Link className="btn btn-ghost" href="/security">How the keys work</Link></div>
            </div>
            <div className="rv d1"><Shot id="link" /></div>
          </div>
        </div>
      </section>

      <Poster title="Nothing moves" em="without your thumb." text="Not a stray tap, not your voice, not an AI, not a stolen device. The trade key is the only yes. Ape responsibly." />

      <section className="sec alt" aria-labelledby="ctrl-title">
        <div className="wrap">
          <div className="split v-center">
            <div className="rv">
              <p className="eyebrow">Hardware</p>
              <h2 id="ctrl-title" className="display">3 controls.<br />Zero excuses.</h2>
              <p className="lead mt-18">A 4-inch touch screen for building orders fast. A knurled thumb wheel to roll and fine-tune. A trade key you can find blind. A power key. A mic for the room and the AI. No order goes out without a press.</p>
              <div className="cta-row"><Link className="btn btn-ghost" href="/hardware">Full spec sheet</Link><Link className="btn btn-ghost" href="/screens">See the screens</Link></div>
            </div>
            <div className="grid rv d1">
              <div className="card"><div className="idx">Top-right edge</div><h3>Thumb wheel</h3><p>Roll: move the focus, adjust a value, flip pages. Press: back — and at the top level, the menu. Set into the shell, rim poking out just enough.</p></div>
              <div className="card neonline"><div className="idx">Right edge</div><h3>Trade key</h3><p>Open / enter. Opens what&apos;s focused; on a ticket it sends. Slightly taller than the shell so you feel it without looking.</p></div>
              <div className="card"><div className="idx">Left edge</div><h3>Power key</h3><p>Short press: screen off, the room keeps playing. Hold two seconds: slide to power off. The mic button lives on the glass.</p></div>
            </div>
          </div>
        </div>
      </section>

      <Tape className="dark mono-tape" items={['Talk your book', 'Then trade it', 'One room per ticker', '12 languages', 'Perps first', 'Hyperliquid · Pacifica · Lighter', 'We never hold your money', 'Revoke any time', 'AI proposes · you confirm', '1,000 units', '$99', 'Ape responsibly']} />

      <section className="sec" aria-labelledby="clip-title">
        <div className="wrap">
          <div className="split v-center rev">
            <div className="rv">
              <p className="eyebrow hot">Accessory · coming soon</p>
              <h2 id="clip-title" className="display">Leave Wi-Fi.<br />Keep the room.</h2>
              <p className="lead mt-18">The 4G back-clip snaps onto six gold contacts on the back. Cellular, its own battery, its own SIM. Magnetic. Sold separately.</p>
              <div className="cta-row"><Link className="btn btn-hot" href="/clip">Watch it snap on</Link></div>
            </div>
            <div className="rv d1">
              <Viewer view="back" clip light="green" controls={false} wide badge="Coming soon" badgeSoon />
            </div>
          </div>
        </div>
      </section>

      <section className="sec alt" aria-labelledby="gen-title">
        <div className="wrap">
          <div className="split">
            <div className="offer rv">
              <div>
                <p className="eyebrow tight">Genesis Edition · limited to 1,000 units</p>
                <span className="price lg">$99</span>
              </div>
              <h2 id="gen-title" className="display md">Be in the room first.</h2>
              <ul>
                <li>PA12 nylon 3D-printed shell, matte, numbered 1–1,000</li>
                <li>Device and USB-C cable</li>
                <li>Ships {SHIP_DATE}, region by region, after certification</li>
                <li>Pre-order is $99 today; full refund any time before your unit ships, no questions</li>
                <li>Stripe Checkout; refunds go back to your original payment method</li>
              </ul>
              <Link className="btn btn-neon" href="/preorder">Pre-order — $99 · refundable</Link>
            </div>
            <div className="rv d1">
              <p className="eyebrow">First wave ships to</p>
              <Regions />
              <p className="note neon mt-22">{CERT_LINE}. Genesis ships after certification, and firmware keeps improving over the air after that.</p>
              <div className="grid g2 mt-22">
                <div className="card"><div className="idx">Venues</div><h3>Three perp DEXs</h3><p>Hyperliquid, Pacifica on Solana, Lighter on Robinhood Chain. All at launch.</p></div>
                <div className="card"><div className="idx">Open source</div><h3>Early</h3><p>Firmware and schematic go public during the pre-order, before Genesis ships. MCP spec too.</p></div>
              </div>
            </div>
          </div>
        </div>
      </section>

      <BuiltInOpen />
    </>
  );
}
