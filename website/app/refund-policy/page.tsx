import type { Metadata } from 'next';
import Link from 'next/link';
import { PageHero } from '@/components/ui';
import { SHIP_DATE } from '@/lib/site';

export const metadata: Metadata = {
  title: 'Refund policy',
  description: 'Degen Walkie refund policy: full refund any time before your unit ships, to your original payment method. After shipment, 30 days unopened.',
};

export default function RefundPolicyPage() {
  return (
    <>
      <PageHero eyebrow="Refunds" title={<>Plain-English<br /><span className="neon">refunds.</span></>} lead="A pre-order is a promise in both directions. Here is ours, in full." />
      <section className="sec" aria-label="Refund policy draft">
        <div className="wrap legal">
          <p className="draft">Draft — not yet reviewed by counsel</p>
          <p>This draft covers refunds for Degen Walkie Genesis Edition pre-orders placed on this site. It will be replaced by a reviewed policy before Genesis ships.</p>

          <h2>Before your unit ships</h2>
          <ul>
            <li><strong>Full refund, any time, no questions.</strong> Until the moment your unit leaves our hands, you can cancel and get all of your $99 back. You do not need to give a reason.</li>
            <li>This applies right up to shipment, which is planned for {SHIP_DATE} and depends on certification. If the ship date slips, the promise does not: it holds until your unit actually ships.</li>
            <li>If you chose &ldquo;Other&rdquo; as your region and we cannot ship to you when your unit is ready, we refund you in full without you having to ask.</li>
          </ul>

          <h2>How to request a refund</h2>
          <ul>
            <li>Email us. Reply to your Stripe receipt, or write to the support address printed on your receipt and on the device&apos;s About screen, from the email address you ordered with.</li>
            <li>Include the receipt number or the last characters of your checkout reference. That is all we need.</li>
            <li>We confirm by email within 3 business days and issue the refund through Stripe.</li>
          </ul>

          <h2>How you get the money back</h2>
          <ul>
            <li>Refunds go to the original payment method, through Stripe. We never ask for bank details or a different card.</li>
            <li>Stripe typically shows the refund on your statement within 5–10 business days, depending on your bank.</li>
            <li>We refund the full amount you paid. Where a currency conversion was applied by your bank, the bank&apos;s rate on the day may differ slightly from the original charge; that difference is outside our control.</li>
          </ul>

          <h2>After your unit ships</h2>
          <ul>
            <li>Standard returns window: <strong>30 days from delivery, unopened</strong>, for a full refund of the device price. Return shipping is at your cost unless the unit arrived damaged or we sent the wrong item.</li>
            <li>Opened units: contact us. A Degen Walkie that has been linked to trading accounts cannot be resold as new, so we handle opened returns case by case, and always for defects under warranty.</li>
            <li>Warranty terms will be published with the reviewed terms before shipment.</li>
          </ul>

          <h2>What a pre-order is not</h2>
          <ul>
            <li>It is not custody of anything. No wallet, keys or funds are created or held by placing a pre-order.</li>
            <li>It is not a security or an investment. You are buying a device at a fixed price, with a full refund available until it ships.</li>
          </ul>

          <h2>Related</h2>
          <p>See the <Link href="/terms">terms</Link> and <Link href="/privacy">privacy</Link> drafts, and the <Link href="/faq">FAQ</Link>.</p>
        </div>
      </section>
    </>
  );
}
