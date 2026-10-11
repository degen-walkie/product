import type { Metadata } from 'next';
import { PageHero } from '@/components/ui';

export const metadata: Metadata = {
  title: 'Privacy',
  description: 'Degen Walkie privacy draft: what voice data the room server processes, what it keeps, what Stripe sees for pre-orders, and why keys and transactions never touch our servers.',
};

export default function PrivacyPage() {
  return (
    <>
      <PageHero eyebrow="Privacy" title={<>Plain-English<br /><span className="neon">privacy.</span></>} lead="What our servers touch, what they don't, and what we keep. Written for a trading terminal that never holds your money, with a voice-room service." />
      <section className="sec" aria-label="Privacy policy draft">
        <div className="wrap legal">
          <p className="draft">Draft — not yet reviewed by counsel</p>
          <p>This draft describes how Degen Walkie (the device), the Degen Walkie room service (our servers) and this website handle data. It will be replaced by a reviewed policy before Genesis ships.</p>

          <h2>The short version</h2>
          <ul>
            <li>We never hold your money. It stays in your own accounts on the trading venues.</li>
            <li>The trading keys you link to the device are stored on the device, encrypted, and never leave it. Orders are signed on the device and sent straight to the venue. Our servers cannot see your keys or place an order.</li>
            <li>Voice does pass through our servers, because that is how speech recognition, the AI and translation work. When you speak an order or a question, our servers return text; the device builds the ticket and waits for your press. We process voice to deliver these features; we do not sell it.</li>
            <li>Pre-order payments are handled by Stripe. We never see your card number.</li>
          </ul>

          <h2>Voice shortcuts and questions to the AI</h2>
          <p>When you hold the mic to speak an order or ask the AI, the device streams your voice to our servers. They convert it to text and, for questions, generate an answer from the market data and the position summary the device sends with the question. The device turns spoken orders into a ticket itself; our servers return only text and cannot send an order. Audio is processed live and not kept.</p>

          <h2>What the room service processes</h2>
          <p>When you hold the mic in a room, the device streams your voice to our servers. The servers convert speech to text, translate the text into the languages of the listeners in that room, synthesise speech, and deliver audio and text to the listeners&apos; devices. To do this we process:</p>
          <ul>
            <li>Your voice audio while you are transmitting, and the text derived from it.</li>
            <li>Your handle, your chosen language, the room you are in, and timestamps.</li>
            <li>A device identifier so rooms can enforce anti-spam rules (message rate limits, cooldowns, the 24-hour listen-only period for new devices, mutes after reports).</li>
            <li>Connection metadata (IP address, connection times) needed to run the service and protect it from abuse.</li>
          </ul>

          <h2>What we keep</h2>
          <ul>
            <li>Audio is processed live. Transmissions are held only as long as needed to transcribe, translate and deliver them, then discarded, except where a report is filed, in which case the reported transmission is kept while the report is reviewed.</li>
            <li>Transcripts are not published and not kept beyond delivery, except as above.</li>
            <li>Anti-spam records (rate-limit counters, mutes, report counts) are kept per device identifier for as long as needed to enforce the rules.</li>
          </ul>

          <h2>Pre-orders and this website</h2>
          <ul>
            <li>When you pre-order, you enter your email and shipping region on this site; they are passed to Stripe to create the checkout session and stored by Stripe with your payment record. Stripe collects your card details directly; we never receive them.</li>
            <li>We receive from Stripe: your email, the region you chose, the amount, the payment status and a reference. We keep this to fulfil, refund and support your order, and delete it on request after the order is closed.</li>
            <li>We email pre-order holders about certification progress and shipping only. No newsletter, no third-party marketing.</li>
            <li>This site sets no analytics or advertising cookies. Stripe sets the cookies it needs for fraud prevention on its own checkout pages.</li>
          </ul>

          <h2>What never touches our servers</h2>
          <ul>
            <li>Your money. We never hold funds; they stay in your accounts on the venues.</li>
            <li>Trading keys and your device password. Agent keys and API keys exist only on the device, in encrypted flash. Your main wallet and its seed phrase never touch the device or us.</li>
            <li>Your orders. The device sends them directly to the venue (for example Hyperliquid, Pacifica or Lighter).</li>
            <li>Your positions and balances. The device reads them from the venue.</li>
          </ul>

          <h2>AI agents and MCP</h2>
          <p>If you connect an AI agent through the MCP interface, that agent (and whoever runs it) may read what you allow: positions, room text, proposal status. Proposals an agent queues are stored on your device until you confirm or dismiss them. We do not operate the agent and do not receive its data unless the agent itself uses our room service.</p>

          <h2>Third parties</h2>
          <p>The device connects to independent networks and venues (for example Hyperliquid, Pacifica, Lighter, GMGN, Solana and Robinhood Chain) using their public interfaces and the API keys you give the device. Their handling of your data, and of any funds you hold with them, is governed by their own terms and policies. We use infrastructure providers to run the room service and this website, and Stripe for payments; they process data on our behalf under contract.</p>

          <h2>Your choices</h2>
          <ul>
            <li>Rooms are optional. Trading works with the room service switched off.</li>
            <li>Radio mode receives only. You can listen without ever transmitting.</li>
            <li>You can change your handle and language, and revoke any trading key, at any time.</li>
            <li>Pre-order emails: reply to any of them to be removed, or to cancel for a full refund before shipment.</li>
          </ul>

          <h2>Contact</h2>
          <p>Privacy questions reach us through the address printed on your receipt and on the device&apos;s About screen. This draft will be updated before shipping.</p>
        </div>
      </section>
    </>
  );
}
