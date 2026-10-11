import type { Metadata } from 'next';
import Viewer from '@/components/Viewer';
import { LedLegend, PageHero, PreorderButtons } from '@/components/ui';
import { CERT_LINE } from '@/lib/site';

export const metadata: Metadata = {
  title: 'Hardware',
  description: 'Degen Walkie Genesis Edition hardware: 64.5 × 112.9 × 10 mm, 4.0-inch IPS touch, ESP32-S3, thumb wheel, trade key, RGB status light, microphone for rooms and the AI, QR camera, six gold contacts for the 4G back-clip. Full spec sheet.',
};

export default function HardwarePage() {
  return (
    <>
      <PageHero eyebrow="Hardware · Genesis Edition" title={<>Built to be <span className="neon">held.</span></>} lead="64.5 × 112.9 × 10 mm. One black slab: the glass front and the matte PA12 shell read as a single surface. One wheel, two keys, one light. Explore the real enclosure CAD below." ctas={<PreorderButtons secondary="See the screens" secondaryHref="/screens" />} />

      <section className="sec" aria-labelledby="explore-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Explore</p><h2 id="explore-title" className="display">Every edge, labelled.</h2><p>Tap a feature to fly the camera to it. Drag to rotate, pinch or scroll to zoom. The model is the Genesis CAD, not an illustration.</p></div>
          <Viewer view="front" light="green" controls features />
        </div>
      </section>

      <section className="sec alt" aria-labelledby="ctrl-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Controls</p><h2 id="ctrl-title" className="display">Exactly three.</h2><p>Everything else is on the 4.0-inch glass, including the mic button — hold it to talk in a room, ask the AI or say an order, big enough to hold while you watch the price. Everything the keys do, touch can do too; the keys are just faster.</p></div>
          <div className="steps">
            <div className="step rv"><div className="n">01</div><h3>Thumb wheel</h3><p>Top-right edge, knurled, set into the shell. Roll to move the focus, adjust a value or flip pages. Press for back; at the top level, the menu. Ø14.5 mm, rim 0.75 mm proud of the wall.</p></div>
            <div className="step rv d1"><div className="n">02</div><h3>Trade key</h3><p>Right edge, centred on the wall. Open / enter: opens the focused item, and on a confirmation sheet it signs. Sits 1.3 mm proud so your finger finds it without looking.</p></div>
            <div className="step rv d2"><div className="n">03</div><h3>Power key</h3><p>Left edge, centred on the wall. Short press: screen off or on, still listening. Hold two seconds: slide to power off.</p></div>
          </div>
        </div>
      </section>

      <section className="sec" aria-labelledby="board-title">
        <div className="wrap">
          <div className="split v-center">
            <div className="rv">
              <p className="eyebrow">Inside</p>
              <h2 id="board-title" className="display">One board.<br />Four <span className="neon">layers.</span></h2>
              <p className="lead mt-18">ESP32-S3 with Wi-Fi and Bluetooth, an AXP2101 power manager, an ES8388 audio codec for the room, and every part on one side so the glass sits flat on the other. Six gold pogo pins on the back for the 4G clip. The schematic goes public during the pre-order; you can check every net yourself.</p>
              <ul className="board-facts mt-22">
                <li><b>4-layer</b> FR-4, 0.8 mm, gold finish</li>
                <li><b>98</b> machine-placed parts, all on one side</li>
                <li><b>6</b> spring pins for the back-clip</li>
              </ul>
            </div>
            <figure className="pcb-figure rv d1">
              <img src="/pcb-genesis.svg" alt="Degen Walkie Genesis mainboard, component side: ESP32-S3 module, power and audio chips, display connectors, gold pogo pins." width={640} height={1100} loading="lazy" />
              <figcaption>Illustrative render of the Genesis mainboard. Copper shown is decorative.</figcaption>
            </figure>
          </div>
        </div>
      </section>

      <section className="sec alt" aria-labelledby="light-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Status light</p><h2 id="light-title" className="display">One nub. Seven meanings.</h2><p>The small nub on the top edge is an RGB light. You can read the device from across the table.</p></div>
          <div className="rv"><LedLegend /></div>
        </div>
      </section>

      <section className="sec" aria-labelledby="edges-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Layout</p><h2 id="edges-title" className="display">Edge map.</h2></div>
          <div className="grid g4">
            <div className="card rv"><div className="idx">Top edge</div><h3>Status light</h3><p>The RGB status light, glowing through a translucent antenna nub.</p></div>
            <div className="card rv d1"><div className="idx">Right edge</div><h3>Wheel · Trade · USB-C</h3><p>Thumb wheel at the top, trade key level with the power key, USB-C low on the same edge.</p></div>
            <div className="card rv d2"><div className="idx">Left edge</div><h3>Power</h3><p>Power key, nothing else.</p></div>
            <div className="card rv d3"><div className="idx">Back</div><h3>Camera · Contacts · Audio</h3><p>QR camera for Wi-Fi setup and account linking, six gold contacts for the 4G back-clip, speaker grille, mic hole.</p></div>
          </div>
        </div>
      </section>

      <section className="sec alt" aria-labelledby="spec-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow">Spec sheet</p><h2 id="spec-title" className="display">Numbers.</h2><p>Genesis Edition hardware. Battery and weight figures are targets until certification units are measured.</p></div>
          <table className="specs rv">
            <tbody>
              <tr><th scope="row">Dimensions</th><td><span className="num">64.5 × 112.9 × 10 mm</span></td></tr>
              <tr><th scope="row">Weight</th><td><span className="num">≈ 110 g</span> (target)</td></tr>
              <tr><th scope="row">Display</th><td><span className="num">4.0&quot;</span> · <span className="num">320 × 480</span> IPS · capacitive touch · glass front flush with the shell</td></tr>
              <tr><th scope="row">Processor &amp; radio</th><td>ESP32-S3 · Wi-Fi <span className="num">2.4 GHz</span> · Bluetooth LE <span className="num">5</span></td></tr>
              <tr><th scope="row">Controls</th><td>Knurled thumb wheel (roll + press) · Trade key · Power key · on-screen mic button (rooms, AI, voice shortcuts)</td></tr>
              <tr><th scope="row">Status light</th><td>RGB nub on the top edge (legend above)</td></tr>
              <tr><th scope="row">Camera</th><td>Rear camera for QR: Wi-Fi setup, account linking</td></tr>
              <tr><th scope="row">Audio</th><td>Speaker · MEMS microphone</td></tr>
              <tr><th scope="row">Port</th><td>USB-C — charging and signed firmware updates</td></tr>
              <tr><th scope="row">Battery</th><td><span className="num">1000 mAh</span> · ≈ <span className="num">3 h</span> talk · <span className="num">1 day</span> standby (target)</td></tr>
              <tr><th scope="row">Accessory contacts</th><td><span className="num">6</span> gold contacts on the back for the magnetic 4G back-clip (coming soon, sold separately)</td></tr>
              <tr><th scope="row">Shell</th><td>PA12 nylon, 3D printed, matte black (Genesis Edition)</td></tr>
              <tr><th scope="row">Security</th><td>Holds revocable trade-only keys, never funds · secure boot · encrypted flash · signed firmware, no downgrade · open-source firmware and schematic</td></tr>
              <tr><th scope="row">Certification</th><td>{CERT_LINE} · ships after certification</td></tr>
              <tr><th scope="row">In the box</th><td>Device · USB-C cable</td></tr>
            </tbody>
          </table>
        </div>
      </section>
    </>
  );
}
