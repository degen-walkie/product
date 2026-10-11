import type { Metadata } from 'next';
import Link from 'next/link';
import { NavDemo, Shot } from '@/components/screens';
import { PageHero, PreorderButtons, Tape } from '@/components/ui';
import { LANGS } from '@/lib/site';

export const metadata: Metadata = {
  title: 'Rooms',
  description: 'A live room for every ticker: hear traders, pros and an AI talk the market, translated into 12 languages. Ask the AI about your positions. Voice shortcuts fill in tickets; the trade key sends them.',
};

export default function RoomsPage() {
  return (
    <>
      <PageHero eyebrow="Rooms" title={<>One room<br />per <span className="neon">ticker.</span></>} lead="Roll to #SOL and you hear the people trading it: traders, pros who host rooms, and an AI that knows your book. In your language, live. When you're ready, the ticket for that market is one press away." ctas={<PreorderButtons secondary="All screens" secondaryHref="/screens" />} />
      <Tape />

      <section className="sec" id="live" aria-labelledby="rooms-title">
        <div className="wrap">
          <div className="sec-head rv">
            <p className="eyebrow">Live rooms</p>
            <h2 id="rooms-title" className="display">By ticker. By number.</h2>
            <p>Room <span className="mono">#SOL</span> is everyone trading SOL. Room <span className="mono">#4471</span> is your crew, by number, nobody else. Hold the mic on the glass to talk; let go to listen. The price strip stays on top the whole time, and it is the focus, so the trade key opens that market&apos;s ticket.</p>
          </div>
          <div className="rv"><NavDemo /></div>
          <div className="grid g3 mt-28">
            <div className="card rv"><div className="idx">Pro rooms</div><h3>Follow the traders you trust</h3><p>Traders can host a room and talk through their book live. You follow, listen, ask, and decide for yourself.</p></div>
            <div className="card rv d1"><div className="idx">Radio mode</div><h3>Screen off. Room on.</h3><p>Tap the power key and the room keeps playing through the speaker, like a radio. The status light still tells you when an order fills.</p></div>
            <div className="card rv d2"><div className="idx">Not advice</div><h3>You make the call</h3><p>Rooms are people talking, not signals. Nothing in a room places an order; only your press does.</p></div>
          </div>
        </div>
      </section>

      <section className="sec alt" id="translate" aria-labelledby="tr-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Translation</p><h2 id="tr-title" className="display">12 languages.<br />Zero subtitles.</h2><p>Hold the mic and speak. Our server runs speech-to-text, translation and text-to-speech, and every listener hears you in their own language — as voice, with your handle and language tag on screen.</p></div>
          <div className="split">
            <div className="card rv pad0 still">
              <dl className="kv pad">
                <div><dt>Spoken · EN</dt><dd style={{ fontWeight: 600 }}>Funding on SOL just flipped negative. Anyone else adding here?</dd></div>
                <div><dt>Heard · 中文</dt><dd lang="zh">SOL 的资金费率刚转负。还有人在这里加仓吗？</dd></div>
                <div><dt>Heard · 日本語</dt><dd lang="ja">SOL のファンディングがちょうどマイナスに転じた。ここで買い増ししてる人いる？</dd></div>
                <div><dt>Heard · Español</dt><dd lang="es">El funding de SOL acaba de pasar a negativo. ¿Alguien más está sumando aquí?</dd></div>
              </dl>
            </div>
            <div className="rv d1">
              <div className="chips" aria-label="Launch languages">{LANGS.map((l) => <span key={l} className="chip on">{l}</span>)}</div>
              <div className="grid mt-22">
                <div className="card"><div className="idx">Server side</div><h3>Voice goes to our server</h3><p>For recognition, translation, synthesis and the AI. Audio is processed, not kept. Details in <Link href="/privacy">Privacy</Link>.</p></div>
                <div className="card neonline"><div className="idx">Never</div><h3>Your keys and orders don&apos;t</h3><p>Orders are signed on the device and go straight to the venue. Our server never sees your keys and cannot place an order.</p></div>
                <div className="card"><div className="idx">Keyboard</div><h3>Type when you can&apos;t talk</h3><p>On-device keyboard with Chinese (pinyin), Japanese (romaji) and Korean (jamo). Typed lines are translated too.</p></div>
              </div>
            </div>
          </div>
        </div>
      </section>

      <section className="sec" id="ask" aria-labelledby="ask-title">
        <div className="wrap">
          <div className="split v-center">
            <div className="rv">
              <p className="eyebrow hot">AI in the room</p>
              <h2 id="ask-title" className="display">Ask your<br />book.</h2>
              <p className="lead mt-18">&ldquo;How far is my SOL long from liquidation?&rdquo; &ldquo;What&apos;s funding on HYPE?&rdquo; &ldquo;What is this room worried about?&rdquo; Hold the mic, ask, and the answer comes back on screen. The AI reads; it never trades.</p>
              <p className="fine mt-18">Agents you run yourself can do the same over MCP: read, ask, propose. The purple light means one is waiting, and only your press can confirm it. <Link href="/roadmap#mcp">Roadmap</Link>.</p>
            </div>
            <div className="rv d1"><Shot id="ask" /></div>
          </div>
        </div>
      </section>

      <section className="sec alt" id="voice" aria-labelledby="voice-title">
        <div className="wrap">
          <div className="split v-center rev">
            <div className="rv">
              <p className="eyebrow">Voice shortcuts</p>
              <h2 id="voice-title" className="display">Hands busy?<br />Say it.</h2>
              <p className="lead mt-18">Touch is the fast path for orders. When your hands are on the keyboard, say it instead: &ldquo;Close half my ETH,&rdquo; &ldquo;Long SOL, five x, five hundred USDC.&rdquo; Voice only fills in the ticket. You check it, fix anything with the wheel, and the trade key sends it.</p>
            </div>
            <div className="rv d1"><Shot id="voice" /></div>
          </div>
        </div>
      </section>

      <section className="sec" aria-labelledby="world-title">
        <div className="wrap">
          <div className="split v-center">
            <div className="rv">
              <p className="eyebrow">World room</p>
              <h2 id="world-title" className="display">Everyone. One room.<br />No spam.</h2>
              <p className="lead mt-18">One group chat for the whole network: handles, language tags, the original line and its translation. The rules run on the device: one message every 10 seconds, a pause after three in a row, and new devices listen for their first 24 hours.</p>
            </div>
            <div className="rv d1"><Shot id="world" animTag="animated · messages arrive live" /></div>
          </div>
        </div>
      </section>
    </>
  );
}
