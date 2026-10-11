import type { Metadata } from 'next';
import Link from 'next/link';
import PreorderForm from '@/components/PreorderForm';
import { PageHero, Regions } from '@/components/ui';
import { stripeConfigured } from '@/lib/stripe';
import { CERT_LINE, SHIP_DATE } from '@/lib/site';

export const metadata: Metadata = {
  title: 'Pre-order',
  description: 'Pre-order a Degen Walkie Genesis Edition for $99. Ships January 2027. Full refund any time before your unit ships, no questions. 1,000 units.',
};

export const dynamic = 'force-dynamic';

export default function PreorderPage() {
  const enabled = stripeConfigured();
  return (
    <>
      <PageHero eyebrow="Pre-order · Genesis Edition" title={<>Get in<br />the <span className="neon">room.</span></>} lead={`Pre-order — $99. Ships ${SHIP_DATE}. Full refund any time before your unit ships, no questions.`} />
      <section className="sec" aria-labelledby="res-title">
        <div className="wrap">
          <div className="split">
            <div className="offer rv">
              <div>
                <p className="eyebrow tight">Genesis Edition · limited to 1,000 units</p>
                <span className="price lg">$99</span>
              </div>
              <h2 id="res-title" className="display md">What you&apos;re pre-ordering.</h2>
              <ul>
                <li>Degen Walkie, Genesis Edition, PA12 nylon shell, numbered 1–1,000</li>
                <li>USB-C cable</li>
                <li>Ships {SHIP_DATE}, after certification ({CERT_LINE})</li>
                <li>One unit per order · paid today via Stripe · refund to your original payment method</li>
                <li>Full refund any time before your unit ships — email us, no questions asked</li>
              </ul>
              <p className="eyebrow tight" style={{ margin: '8px 0 0' }}>First wave ships to</p>
              <Regions />
              <p className="fine">Chose &ldquo;Other&rdquo;? We refund in full if we can&apos;t ship to you when your unit is ready. <Link href="/refund-policy">Refund policy</Link>.</p>
            </div>
            <div className="rv d1">
              <PreorderForm enabled={enabled} />
            </div>
          </div>
        </div>
      </section>
    </>
  );
}
