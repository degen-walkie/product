import Stripe from 'stripe';

/* Server-only Stripe helpers. The site runs in "pre-orders open soon" mode until a real secret key is set. */

function clean(v: string | undefined): string {
  return (v || '').trim();
}

export function stripeConfigured(): boolean {
  const k = clean(process.env.STRIPE_SECRET_KEY);
  return k.startsWith('sk_') && !k.includes('REPLACE_ME');
}

export function stripeLive(): boolean {
  return clean(process.env.STRIPE_SECRET_KEY).startsWith('sk_live_');
}

let client: Stripe | null = null;
export function getStripe(): Stripe | null {
  if (!stripeConfigured()) return null;
  if (!client) {
    client = new Stripe(clean(process.env.STRIPE_SECRET_KEY), {
      appInfo: { name: 'degen-walkie-site', version: '3.0.0' },
    });
  }
  return client;
}

export function priceId(): string | null {
  const p = clean(process.env.STRIPE_PRICE_ID);
  return p.startsWith('price_') ? p : null;
}

export function webhookSecret(): string | null {
  const s = clean(process.env.STRIPE_WEBHOOK_SECRET);
  return s.startsWith('whsec_') && !s.includes('REPLACE_ME') ? s : null;
}
