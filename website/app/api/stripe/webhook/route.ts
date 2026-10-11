import { NextResponse } from 'next/server';
import type Stripe from 'stripe';
import { getStripe, webhookSecret } from '@/lib/stripe';

export const runtime = 'nodejs';
export const dynamic = 'force-dynamic';

/* Stripe webhook. Verifies the signature with STRIPE_WEBHOOK_SECRET and logs checkout.session.completed.
   Point the Dashboard webhook (or `stripe listen --forward-to localhost:3000/api/stripe/webhook`) here. */
export async function POST(req: Request) {
  const stripe = getStripe();
  const secret = webhookSecret();
  if (!stripe || !secret) {
    return NextResponse.json({ error: 'Webhook not configured (STRIPE_SECRET_KEY / STRIPE_WEBHOOK_SECRET).' }, { status: 503 });
  }
  const sig = req.headers.get('stripe-signature');
  if (!sig) return NextResponse.json({ error: 'Missing stripe-signature header.' }, { status: 400 });

  // The raw body is required for signature verification — do not parse it as JSON first.
  const payload = await req.text();
  let event: Stripe.Event;
  try {
    event = stripe.webhooks.constructEvent(payload, sig, secret);
  } catch (e) {
    const msg = e instanceof Error ? e.message : 'bad signature';
    console.warn('[webhook] signature failed:', msg);
    return NextResponse.json({ error: `Webhook signature verification failed: ${msg}` }, { status: 400 });
  }

  switch (event.type) {
    case 'checkout.session.completed': {
      const s = event.data.object as Stripe.Checkout.Session;
      console.log('[webhook] checkout.session.completed', {
        id: s.id,
        email: s.customer_details?.email ?? s.customer_email,
        amount_total: s.amount_total,
        currency: s.currency,
        region: s.metadata?.region,
        payment_status: s.payment_status,
      });
      // TODO(pre-orders): persist the order here (e.g. Vercel Postgres / KV / Supabase):
      //   { sessionId, paymentIntent, email, region, amount, createdAt, unitNumber? }
      // TODO(email): send the confirmation + refund-promise email (Resend / Postmark) from here, not from the success page.
      break;
    }
    case 'charge.refunded':
      console.log('[webhook] charge.refunded', (event.data.object as Stripe.Charge).id);
      // TODO: mark the order refunded.
      break;
    default:
      // Unhandled event types are fine; Stripe only needs a 2xx.
      break;
  }
  return NextResponse.json({ received: true });
}
