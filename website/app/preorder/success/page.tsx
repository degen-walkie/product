import type { Metadata } from 'next';
import Link from 'next/link';
import { PageHero } from '@/components/ui';
import { getStripe } from '@/lib/stripe';
import { SHIP_DATE } from '@/lib/site';

export const metadata: Metadata = { title: 'Pre-order confirmed', robots: { index: false } };
export const dynamic = 'force-dynamic';

interface Summary { email: string | null; amount: string; region: string; status: string; ref: string; name: string }

async function loadSummary(sessionId: string | undefined): Promise<Summary | null> {
  const stripe = getStripe();
  if (!stripe || !sessionId || !/^cs_(test|live)_[A-Za-z0-9]+$/.test(sessionId)) return null;
  try {
    const s = await stripe.checkout.sessions.retrieve(sessionId, { expand: ['line_items'] });
    const amount = ((s.amount_total ?? 0) / 100).toLocaleString('en-US', { style: 'currency', currency: (s.currency || 'usd').toUpperCase() });
    return {
      email: s.customer_details?.email ?? s.customer_email ?? null,
      amount,
      region: s.metadata?.region || '—',
      status: s.payment_status,
      ref: s.id.slice(-8).toUpperCase(),
      name: s.line_items?.data[0]?.description || 'Degen Walkie — Genesis Edition (pre-order)',
    };
  } catch (e) {
    console.warn('[success] could not retrieve session', e instanceof Error ? e.message : e);
    return null;
  }
}

export default async function SuccessPage({ searchParams }: { searchParams: { session_id?: string } }) {
  const summary = await loadSummary(searchParams.session_id);
  return (
    <>
      <PageHero eyebrow="Pre-order · confirmed" title={<>You&apos;re in<br />the <span className="neon">room.</span></>} lead={`Thank you. Your Genesis Edition ships ${SHIP_DATE}. Full refund any time before it ships, no questions.`} />
      <section className="sec" aria-labelledby="ok-title">
        <div className="wrap">
          <div className="split">
            <div className="rv">
              <div className="success" role="status">
                <h3 id="ok-title">Order received.</h3>
                {summary ? (
                  <dl className="order-summary">
                    <div><dt>Item</dt><dd>{summary.name}</dd></div>
                    <div><dt>Paid</dt><dd>{summary.amount}</dd></div>
                    <div><dt>Status</dt><dd>{summary.status}</dd></div>
                    <div><dt>Ships to</dt><dd>{summary.region}</dd></div>
                    <div><dt>Email</dt><dd>{summary.email ?? '—'}</dd></div>
                    <div><dt>Reference</dt><dd>{summary.ref}</dd></div>
                  </dl>
                ) : (
                  <p>Stripe has emailed you a receipt. Keep it: the receipt number is your pre-order reference.</p>
                )}
                <p>Shipping address: we email you before your unit ships to collect it, so a move between now and {SHIP_DATE} costs you nothing.</p>
              </div>
            </div>
            <div className="rv d1 grid">
              <div className="card neonline">
                <div className="idx">Refund promise</div>
                <h3>Full refund any time before shipment.</h3>
                <p>Reply to your receipt email, or write to the address on the <Link href="/refund-policy">refund policy</Link> page, and we refund the full $99 to your original payment method. No questions. After shipment, 30 days unopened.</p>
              </div>
              <div className="card">
                <div className="idx">What happens next</div>
                <h3>Certification, then units.</h3>
                <p>FCC, CE, UKCA, TELEC and IMDA are in progress. We email progress to pre-order holders only; no newsletter. Your unit number is assigned at shipment.</p>
              </div>
              <div className="cta-row">
                <Link className="btn btn-ghost" href="/screens">See the screens</Link>
                <Link className="btn btn-ghost" href="/">Home</Link>
              </div>
            </div>
          </div>
        </div>
      </section>
    </>
  );
}
