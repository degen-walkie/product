'use client';

import Link from 'next/link';
import { useState } from 'react';
import { isValidEmail, isValidRegion, PREORDER_LINE, REGION_OPTIONS } from '@/lib/site';

/* Collects email + region, then hands off to Stripe Checkout via /api/checkout. */
export default function PreorderForm({ enabled }: { enabled: boolean }) {
  const [email, setEmail] = useState('');
  const [region, setRegion] = useState('');
  const [badEmail, setBadEmail] = useState(false);
  const [badRegion, setBadRegion] = useState(false);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [soon, setSoon] = useState(!enabled);

  async function submit(e: React.FormEvent) {
    e.preventDefault();
    const okE = isValidEmail(email), okR = isValidRegion(region);
    setBadEmail(!okE);
    setBadRegion(!okR);
    if (!okE || !okR) return;
    setBusy(true);
    setError(null);
    try {
      const res = await fetch('/api/checkout', { method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify({ email: email.trim(), region }) });
      const data = (await res.json().catch(() => ({}))) as { url?: string; error?: string; code?: string };
      if (res.status === 503) { setSoon(true); return; }
      if (!res.ok || !data.url) { setError(data.error || 'Could not start checkout.'); return; }
      window.location.assign(data.url);
    } catch {
      setError('Network error. Please try again.');
    } finally {
      setBusy(false);
    }
  }

  if (soon) {
    return (
      <div className="success soon" role="status">
        <h3>Pre-orders open soon.</h3>
        <p>Payments are not switched on yet. Nothing has been charged. Check back shortly — the price stays $99 and the refund promise stays the same: full refund any time before your unit ships.</p>
        <p><Link className="btn btn-ghost btn-sm" href="/refund-policy">Read the refund policy</Link></p>
      </div>
    );
  }

  return (
    <form className="form" onSubmit={submit} noValidate>
      <div className={`field${badEmail ? ' invalid' : ''}`}>
        <label htmlFor="email">Email</label>
        <input id="email" name="email" type="email" inputMode="email" autoComplete="email" placeholder="you@example.com" required value={email} onChange={(e) => { setEmail(e.target.value); if (isValidEmail(e.target.value)) setBadEmail(false); }} disabled={busy} />
        <p className="field-error">Enter a valid email address.</p>
      </div>
      <div className={`field${badRegion ? ' invalid' : ''}`}>
        <label htmlFor="region">Shipping region</label>
        <select id="region" name="region" required value={region} onChange={(e) => { setRegion(e.target.value); if (isValidRegion(e.target.value)) setBadRegion(false); }} disabled={busy}>
          <option value="" disabled>Select your region</option>
          {REGION_OPTIONS.map((r) => (
            <option key={r} value={r}>{r}</option>
          ))}
        </select>
        <p className="field-error">Choose a shipping region.</p>
      </div>
      <div className="field">
        <label htmlFor="qty">Quantity</label>
        <div className="qty" id="qty"><span>1 × Genesis Edition</span><small>one per order</small></div>
      </div>
      {error ? <p className="form-error" role="alert">{error}</p> : null}
      <button className="btn btn-neon" type="submit" disabled={busy}>{busy ? 'Opening checkout…' : 'Pre-order — $99 · refundable'}</button>
      <p className="form-note">{PREORDER_LINE}</p>
      <p className="form-note">Payment is handled by Stripe Checkout on the next page. We ask for your shipping address before your unit ships. See the <Link href="/refund-policy">refund policy</Link>.</p>
    </form>
  );
}
