import type { Metadata } from 'next';
import { Pair, Shot, SpotDemo } from '@/components/screens';
import { PageHero, PreorderButtons } from '@/components/ui';
import { VENUES } from '@/lib/site';

export const metadata: Metadata = {
  title: 'Trade',
  description: 'Build the order by touch, press the trade key to send. Perps on Hyperliquid, Pacifica (Solana) and Lighter (Robinhood Chain); spot and memes on the side. Your money stays on the venue; the device holds a revocable trade-only key. Press the trade key to confirm.',
};

export default function TradePage() {
  return (
    <>
      <PageHero eyebrow="Trade" title={<>The room is<br />the <span className="neon">order.</span></>} lead="You're in #HYPE, listening. Press the trade key and the HYPE ticket opens on your Hyperliquid account. Tap in the order, review the sheet, press again, filled. Your money never leaves the venue; the device only holds a key that can trade." ctas={<PreorderButtons secondary="Why we never hold your money" secondaryHref="/security" />} />

      <section className="sec" aria-labelledby="flow-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Flow</p><h2 id="flow-title" className="display">Press. Review. Press.</h2></div>
          <div className="steps">
            <div className="step rv"><div className="n">01</div><h3>Open from the room</h3><p>In any room or chart, the right-edge trade key opens the ticket for that market. Or tap a position to add, close half or close all.</p></div>
            <div className="step rv d1"><div className="n">02</div><h3>Tap the ticket</h3><p>Long or short, market or limit, leverage, size presets, TP / SL, reduce-only: a few taps on the glass. The wheel fine-tunes any number. The trade key opens the confirmation sheet.</p></div>
            <div className="step rv d2"><div className="n hot">03</div><h3>Confirm on device</h3><p>On the sheet the same key confirms. The order is signed on the device with your trade-only key. Blue light while it sends, green for 3 s when filled.</p></div>
          </div>
          <div className="rv mt-36"><SpotDemo /></div>
        </div>
      </section>

      <section className="sec alt" aria-labelledby="keys-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Trade-only keys</p><h2 id="keys-title" className="display">Can trade.<br />Can&apos;t withdraw.</h2><p>Your money stays on the venue, in your account. You approve a key for the device from your main wallet, once. That key can place and cancel orders. On Hyperliquid the protocol itself will not let it withdraw or change its own permissions. Revoke it whenever you like and the device is cut off.</p></div>
          <div className="wallets rv">
            <div className="wallet trading"><div className="idx">Your main wallet</div><h3>Owns</h3><ul><li>Holds the account on the venue</li><li>The only thing that can withdraw</li><li>Approves and revokes the device</li></ul></div>
            <div className="arrow"><b>→</b>approve once<br />revoke any time</div>
            <div className="wallet vault"><div className="idx">Degen Walkie</div><h3>Trades</h3><ul><li>Places and cancels orders</li><li>Cannot withdraw</li><li>Cannot change its own permissions</li></ul></div>
          </div>
          <div className="screens rv d1 mt-36"><Shot id="accounts" /><Shot id="link" /><Shot id="revoke" /></div>
        </div>
      </section>

      <section className="sec" aria-labelledby="venues-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Venues</p><h2 id="venues-title" className="display">Three perp DEXs.</h2><p>One on its own chain, one on Solana, one on Robinhood Chain. Every one keeps your money in your own account and gives the device a key you can revoke.</p></div>
          <div className="grid g3 rv">
            {VENUES.map((v) => (
              <div className={`card${v.status === 'At launch' ? ' neonline' : ''}`} key={v.name}>
                <div className="idx">{v.status} · {v.market}</div>
                <h3>{v.name}</h3>
                <p>{v.how}</p>
                {v.note ? <p className="fine">{v.note}</p> : null}
              </div>
            ))}
          </div>
          <p className="fine mt-28">Spot runs on Hyperliquid with the same key. Memes are covered through GMGN. Both are extras; perps are the point.</p>
        </div>
      </section>

      <section className="sec alt" aria-labelledby="mkts-title">
        <div className="wrap">
          <div className="split">
            <div className="rv">
              <p className="eyebrow">Markets</p>
              <h2 id="mkts-title" className="display">Perps first.</h2>
              <dl className="kv mt-22">
                <div><dt>Perps</dt><dd>Hyperliquid, Pacifica (Solana) and Lighter (Robinhood Chain), all at launch. <small className="mute">Eligible regions only; off by default until you confirm your jurisdiction on first boot.</small></dd></div>
                <div><dt>Spot</dt><dd>Hyperliquid spot, through the same key. A side dish.</dd></div>
                <div><dt>Memes</dt><dd>Covered through GMGN, so the rooms you talk in are rooms you can trade in. <small className="mute">GMGN is a hosted meme platform, not DeFi: funds there are held by GMGN under its terms, never by us.</small></dd></div>
                <div><dt>Input</dt><dd>On-device keyboard with Chinese (pinyin), Japanese (romaji), Korean (jamo). Touch-first tickets; voice shortcuts through the mic. QR scan with the back camera to link accounts.</dd></div>
              </dl>
            </div>
            <Pair><Shot id="perps" small /><Shot id="spot" small /></Pair>
          </div>
        </div>
      </section>
    </>
  );
}
