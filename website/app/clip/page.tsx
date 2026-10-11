import type { Metadata } from 'next';
import Link from 'next/link';
import ClipScrollSection from '@/components/ClipScrollSection';
import { PageHero } from '@/components/ui';

export const metadata: Metadata = {
  title: '4G Back-Clip',
  description: 'The Degen Walkie 4G back-clip: magnetic, snaps onto six gold contacts, adds cellular, its own battery and SIM. Coming soon. Accessory, sold separately.',
};

export default function ClipPage() {
  return (
    <>
      <PageHero hot eyebrow="Accessory · coming soon" title={<>Leave Wi-Fi.<br />Keep the <span className="hot">room.</span></>} lead="A magnetic back-clip with 4G, its own battery and its own SIM. It snaps onto the six gold contacts on the back cover and the device just has signal. Scroll: the real CAD snaps together below." ctas={<><Link className="btn btn-hot" href="/preorder">Pre-order a Genesis — $99</Link><Link className="btn btn-ghost" href="/hardware">Device hardware</Link></>} />

      <section className="sec" aria-labelledby="snap-title">
        <div className="wrap">
          <h2 id="snap-title" className="sr">Clip assembly animation</h2>
          <ClipScrollSection />
        </div>
      </section>

      <section className="sec alt" aria-labelledby="clipv-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow hot">What it adds</p><h2 id="clipv-title" className="display">Snap on.<br />Go outside.</h2></div>
          <div className="grid g4">
            <div className="card hotline rv"><div className="idx hot">Cellular</div><h3>4G LTE</h3><p>Rooms, prices and trades without Wi-Fi. The device talks to the clip over the contacts.</p></div>
            <div className="card rv d1"><div className="idx hot">Power</div><h3>Its own battery</h3><p>The clip carries its own cell, so cellular never eats the device&apos;s 1000 mAh.</p></div>
            <div className="card rv d2"><div className="idx hot">Attach</div><h3>Magnetic + 6 pogo contacts</h3><p>Magnets hold it; six spring pins on the clip meet six gold pads on the back cover. No cable.</p></div>
            <div className="card rv d3"><div className="idx hot">SIM</div><h3>Your SIM, your plan</h3><p>Nano-SIM door on the clip. Any carrier.</p></div>
          </div>
        </div>
      </section>

      <section className="sec" aria-labelledby="clipq-title">
        <div className="wrap">
          <div className="sec-head rv"><p className="eyebrow hot">Status</p><h2 id="clipq-title" className="display">Coming soon.</h2><p>The clip is an accessory and is not included with Genesis. Genesis devices ship with the contacts and the firmware hooks; the clip follows. Pricing and timing on the <Link href="/roadmap">roadmap</Link>.</p></div>
          <div className="grid g3">
            <div className="card rv"><div className="idx">Size</div><h3>60 × 78 × 6 mm</h3><p>Sits inside the back cover&apos;s footprint, centred low so the camera stays clear.</p></div>
            <div className="card rv d1"><div className="idx">Genesis</div><h3>Contacts included</h3><p>Every Genesis unit has the six gold pads and the magnet steel. Nothing to retrofit.</p></div>
            <div className="card rv d2"><div className="idx">Firmware</div><h3>Arrives in an update</h3><p>Clip support lands in a signed firmware update before the clip ships.</p></div>
          </div>
        </div>
      </section>
    </>
  );
}
