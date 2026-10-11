import type { Metadata } from 'next';
import Link from 'next/link';
import { PageHero } from '@/components/ui';

export const metadata: Metadata = { title: 'Checkout cancelled', robots: { index: false } };

export default function CancelPage() {
  return (
    <>
      <PageHero eyebrow="Pre-order · cancelled" title={<>Nothing<br /><span className="neon">charged.</span></>} lead="You left Stripe Checkout before paying. No card was charged and nothing was reserved. Come back whenever you like; Genesis stays at $99 while units last." />
      <section className="sec" aria-label="Next steps">
        <div className="wrap">
          <div className="cta-row rv">
            <Link className="btn btn-neon" href="/preorder">Try again — $99 · refundable</Link>
            <Link className="btn btn-ghost" href="/faq">Questions? FAQ</Link>
          </div>
        </div>
      </section>
    </>
  );
}
