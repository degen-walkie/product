import { NextResponse } from 'next/server';
import { getStripe, priceId } from '@/lib/stripe';
import { isValidEmail, isValidRegion, PRICE_CENTS, SHIP_DATE, siteUrl } from '@/lib/site';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

/* POST { email, region } → { url } (Stripe Checkout, mode: payment, one $99 Genesis pre-order).
   503 with a friendly message while Stripe is not configured. */
export async function POST(req: Request) {
  const stripe = getStripe();
  if (!stripe) {
    return NextResponse.json({ error: 'Pre-orders open soon. Payments are not switched on yet.', code: 'stripe_unconfigured' }, { status: 503 });
  }

  let body: { email?: unknown; region?: unknown } = {};
  try {
    body = await req.json();
  } catch {
    return NextResponse.json({ error: 'Send JSON with email and region.' }, { status: 400 });
  }
  const email = typeof body.email === 'string' ? body.email.trim() : '';
  const region = body.region;
  if (!isValidEmail(email)) return NextResponse.json({ error: 'Enter a valid email address.' }, { status: 400 });
  if (!isValidRegion(region)) return NextResponse.json({ error: 'Choose a shipping region.' }, { status: 400 });

  const base = siteUrl();
  const price = priceId();

  try {
    const session = await stripe.checkout.sessions.create({
      mode: 'payment',
      customer_email: email,
      customer_creation: 'always',
      line_items: [
        price
          ? { price, quantity: 1 }
          : {
              quantity: 1,
              price_data: {
                currency: 'usd',
                unit_amount: PRICE_CENTS,
                product_data: {
                  name: 'Degen Walkie — Genesis Edition (pre-order)',
                  description: `Numbered PA12 nylon shell, USB-C cable. Ships ${SHIP_DATE}. Full refund any time before your unit ships.`,
                },
              },
            },
      ],
      // Quantity is fixed at 1 per order (Genesis is limited to 1,000 units).
      metadata: { region: String(region), product: 'genesis', site: 'v3' },
      payment_intent_data: { metadata: { region: String(region), product: 'genesis' } },
      success_url: `${base}/preorder/success?session_id={CHECKOUT_SESSION_ID}`,
      cancel_url: `${base}/preorder/cancel`,
      allow_promotion_codes: false,
    });
    return NextResponse.json({ url: session.url, id: session.id });
  } catch (e) {
    const msg = e instanceof Error ? e.message : 'Stripe error';
    console.error('[checkout]', msg);
    return NextResponse.json({ error: 'Could not start checkout. Please try again in a moment.' }, { status: 502 });
  }
}

export async function GET() {
  return NextResponse.json({ enabled: !!getStripe(), price: PRICE_CENTS, currency: 'usd' });
}
