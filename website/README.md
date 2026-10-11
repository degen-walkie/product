# Degen Walkie — website

Source of [degenwalkie.trade](https://degenwalkie.trade/). Next.js 14 (App Router, TypeScript, React 18) with a three.js
viewer of the real enclosure CAD, the on-device UI rendered in HTML, and a Stripe Checkout pre-order flow ($99, refundable).
Deploys to Vercel as-is.

## Run locally

```bash
npm install
cp .env.example .env.local     # optional: without it the site runs in "pre-orders open soon" mode
npm run dev                    # http://localhost:3000
```

Other scripts: `npm run build` (production build, must pass), `npm start` (serve the build), `npm run typecheck` (`tsc --noEmit`).

Node 18.17+ (tested with Node 22). The STL models live in `public/models/` (2.3 MB total) and are loaded at runtime by `STLLoader`.

## Routes

| Route | What |
| --- | --- |
| `/` | Home: CAD viewer with a rotating set of on-device screens (live room, chart, positions, news, voice shortcut), how it works, features, "not another cold wallet", controls, 4G clip teaser, Genesis offer |
| `/talk` | Rooms: live rooms per ticker, translation in 12 languages, AI in the room, voice shortcuts, world room |
| `/trade` | Touch-first order flow, trade-only keys, venues (Hyperliquid, Pacifica, Lighter), markets |
| `/security` | Why we never hold your money, key model, AI proposals, power key, open-source plan |
| `/hardware` | Explorable CAD with feature callouts, controls, mainboard render, LED legend, edge map, spec sheet |
| `/screens` | Control rules (roll / back / open), animated demos, static screens, LED legend |
| `/clip` | Scroll-driven assembly of the 4G back-clip |
| `/roadmap`, `/faq` | Timeline, questions |
| `/preorder` | Email + region form → Stripe Checkout (`/api/checkout`) |
| `/preorder/success`, `/preorder/cancel` | Order summary / nothing charged |
| `/refund-policy`, `/privacy`, `/terms` | Drafts, marked "not yet reviewed by counsel" |
| `/api/checkout` | `POST {email, region}` → `{url}`; `503` while Stripe is unconfigured; `GET` → `{enabled}` |
| `/api/stripe/webhook` | Verifies `stripe-signature`, logs `checkout.session.completed` |
| `/opengraph-image` | Generated OG image |

## Environment variables

Copy `.env.example` to `.env.local`. All are optional for a local run.

| Variable | Purpose |
| --- | --- |
| `STRIPE_SECRET_KEY` | `sk_test_…` or `sk_live_…`. Server-only. While missing or left at the placeholder, `/api/checkout` returns 503 and `/preorder` shows "Pre-orders open soon". |
| `NEXT_PUBLIC_STRIPE_PUBLISHABLE_KEY` | `pk_test_…` / `pk_live_…`. Informational for now (Checkout is hosted by Stripe). |
| `STRIPE_PRICE_ID` | Optional `price_…` for the $99 product. If empty, the API uses inline `price_data` ($99.00 USD). |
| `STRIPE_WEBHOOK_SECRET` | `whsec_…` from the Dashboard webhook (or `stripe listen`). Required for `/api/stripe/webhook`. |
| `NEXT_PUBLIC_SITE_URL` | Public origin, no trailing slash. Used for Stripe success/cancel URLs and OG metadata. |
| `NEXT_PUBLIC_GITHUB_URL`, `NEXT_PUBLIC_X_URL` | Links for the "Built in the open" section on `/`. The section is hidden if both are empty. |

## Deploy to Vercel in 5 steps

1. Push this folder to a Git repository (GitHub, GitLab or Bitbucket).
2. In Vercel, **Add New → Project**, import the repository. Framework preset: Next.js (auto-detected). No build settings to change.
3. Under **Settings → Environment Variables**, add the variables above. At minimum set `NEXT_PUBLIC_SITE_URL` to your production URL (e.g. `https://degenwalkie.trade`); add the Stripe keys when you are ready to take payments.
4. Click **Deploy**. Every push to the default branch redeploys; pull requests get preview URLs (set `NEXT_PUBLIC_SITE_URL` per environment if previews should return to themselves).
5. In the Stripe Dashboard, **Developers → Webhooks → Add endpoint**: `https://<your-domain>/api/stripe/webhook`, events `checkout.session.completed` (and `charge.refunded`). Copy the signing secret into `STRIPE_WEBHOOK_SECRET` on Vercel and redeploy.

## Stripe: test → live

1. Test first with `sk_test_…` / `pk_test_…` and card `4242 4242 4242 4242`. Locally, run `stripe listen --forward-to localhost:3000/api/stripe/webhook` and put the printed `whsec_…` in `.env.local`.
2. In the Stripe Dashboard, complete account activation (business details, payout bank). Optionally create a Product "Degen Walkie — Genesis Edition (pre-order)" with a one-time $99.00 price and copy its `price_…` id.
3. Switch the Dashboard toggle to **Live**, create a live webhook endpoint for the same URL and copy its signing secret.
4. On Vercel, replace the four Stripe variables with the live values (`sk_live_…`, `pk_live_…`, the live `price_…` if used, the live `whsec_…`) in the Production environment only. Keep test keys in Preview.
5. Redeploy. Place one real $99 order and refund it from the Dashboard to confirm the refund path; the webhook logs `charge.refunded`.

Refunds are issued from the Stripe Dashboard (Payments → Refund). There is no database yet: the webhook route marks where to persist orders and send the confirmation email.

## Project shape

```
app/            routes (App Router), globals.css, layout with next/font (Archivo wdth axis, Inter, JetBrains Mono)
app/api/        checkout + stripe/webhook route handlers (Node runtime)
components/     Nav, Footer, ui (hero, tape, legend), Viewer/DeviceViewer (three.js + OrbitControls),
                ClipScroll (scroll-driven), PreorderForm, BuiltInOpen, screens/ (device frame + 18 on-device screens)
lib/            site.ts (facts, regions, copy), stripe.ts (server helpers), three/device.ts (CAD rig, materials,
                screen texture, status-light timeline)
public/models/  the seven Genesis STLs
```

### 3D notes

- `three` ^0.170, `STLLoader`, `OrbitControls` (damping, no pan; auto-rotate on the hero pauses while you drag and resumes after 3 s). Preset buttons animate the camera.
- Parts are placed in the CAD frame (X width, Y up, Z thickness; glass at z = 0). Keys are centred on the edges: power key at (−30.65, 30, −5), trade key at (30.65, −20, −5), status light at (−9, 55.75, −5). Six gold pogo discs at x = 13.4 + 2.54·i, y = −20; camera window at (3, 48.6).
- Materials: shell and back are the same matte black (#121417) so the rim merges with the glass's ink border; a soft rim-light shader term and faint edge lines keep the silhouette readable. Wheel dark metallic, trade key neon, status light emissive cycling the legend.
- `/clip` reads scroll progress (passive `scroll` listener, no scroll-jacking): the clip travels from ~45 mm behind the device, eases in with a small overshoot "click" at 68 % of the section, then the assembly rotates together.

### On-device UI model

Roll = move (adjust a value on numeric fields, flip pages in paged views). Wheel press = back (closes sheets, up one level; at the top level it toggles the side menu). Trade key = open / enter (opens the focused item; on a confirmation sheet it signs). Power key: short press screen off/on, hold 2 s for slide-to-power-off. Every screen has one focus (neon outline) and the three-slot hint bar. Everything the keys do, touch can do too.

## License

MIT, see [`LICENSE`](LICENSE). The device renders and STL models in `public/` show the Degen Walkie product design; the
name and logo are not licensed for use on other products.
