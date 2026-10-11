import type { Metadata } from 'next';
import { PageHero, PreorderButtons } from '@/components/ui';

export const metadata: Metadata = {
  title: 'FAQ',
  description: 'Degen Walkie questions: shipping and certification, pre-orders and refunds, controls, 4G, AI agents, venues, where your money sits, lost devices, status light, open source, languages, password wipe, box contents.',
};

const FAQ: [string, string][] = [
  ['When does it ship?', 'January 2027, after certification. FCC, CE, UKCA, TELEC and IMDA are in progress. The first wave ships to the United States, Canada, the European Union, the United Kingdom, Japan, Hong Kong and Singapore, with more regions as approvals land.'],
  ['What does a pre-order commit me to?', 'You pay $99 today through Stripe Checkout. You can cancel for a full refund any time before your unit ships, no questions asked; the refund goes back to your original payment method. After shipment, standard returns: 30 days unopened. See the refund policy.'],
  ['What are the physical controls?', 'Exactly three: a knurled thumb wheel on the top-right edge, a trade key on the right edge, and a power key on the left edge. Roll the wheel to move the focus, adjust a value or flip pages. Press the wheel for back (at the top level it opens the side menu). The trade key opens the focused item, and on a ticket it sends. Orders are built mostly by touch; the mic is a big on-screen button for rooms, the AI and voice shortcuts. Everything the keys do, touch can do too.'],
  ['How do I place an order?', 'By touch, mostly: long or short, market or limit, leverage, size presets and TP / SL take a few taps, and the thumb wheel fine-tunes any number. The physical trade key sends it. When your hands are busy you can say it instead (“close half my ETH”); voice only fills in the ticket, and the trade key still sends it.'],
  ['What are the rooms for?', 'Every ticker has a live room where traders, pros who host rooms and an AI talk the market, translated into 12 languages. You can listen, talk back, or ask the AI about your own positions. Rooms are optional and never touch your keys or orders.'],
  ['Why not a cold wallet?', 'Cold wallets are for holding; this is for trading. Keep yours: it approves a trading key for Degen Walkie on the venue once, then goes back in the drawer. We never hold your money and the device never holds your wallet.'],
  ['What is the camera for?', 'Scanning QR codes: your Wi-Fi network during setup, and account-linking codes from a venue. Nothing else.'],
  ['Does it have 4G?', 'Genesis connects over Wi-Fi 2.4 GHz and Bluetooth LE 5. The 4G back-clip is a magnetic accessory that snaps onto the six gold contacts on the back and adds cellular, its own battery and a SIM. It is coming soon and sold separately.'],
  ['Can an AI agent trade for me?', "It can propose. The MCP interface lets an agent read positions and room context and queue an order proposal. The purple status light tells you one is waiting. Every proposal needs a physical press of the trade key on the device. AI can propose; only your thumb can confirm."],
  ['What can I trade, and where?', 'Perps first: Hyperliquid, Pacifica on Solana and Lighter on Robinhood Chain, all at launch (key-permission details for Pacifica and Lighter are still being confirmed with them). Spot on Hyperliquid and meme coins through GMGN come along too. Perpetuals are available in eligible regions only, are off by default, and unlock after you confirm your jurisdiction on first boot.'],
  ['Where is my money? Do you hold it?', "Never. Your money stays in your own account on the venue. For Hyperliquid you approve an agent key for the device from your main wallet; it can trade, and the protocol does not let it withdraw or change its permissions. For meme coins, funds sit in your GMGN account, held by GMGN under its terms, and the device uses a GMGN API key. There is no Degen Walkie wallet and nothing to deposit with us."],
  ['What if I lose the device or it gets stolen?', 'Revoke its key from your main wallet on Hyperliquid, and its API key in GMGN. Until then, the worst it can do is place trades; on Hyperliquid it cannot withdraw. Six wrong passwords also wipe the keys on the device.'],
  ['Do you see my trades?', 'Orders go from the device to the venue. The only traffic through our server is room audio for speech recognition and translation, and the room service itself.'],
  ['What does the status light mean?', "Blue blink: transaction sending or awaiting confirmation. Green for 3 s: confirmed. Red triple-blink: failed. Red blink every 5 s: battery under 10%. Yellow: charging. Green: full. Purple slow blink: an AI agent's proposal is waiting for your press."],
  ['Is it open source?', 'The parts that matter for trust, early: firmware and schematic go public during the pre-order, before Genesis ships, together with the MCP interface spec. Enclosure CAD follows when Genesis ships. The PCB layout and the server side (voice recognition, AI, translation and rooms) stay closed.'],
  ['What languages?', 'English, Chinese, Japanese, Korean, Arabic, Spanish, Portuguese, German, French, Polish, Russian, Italian. Translation runs on our server; you hear the room in your language. The on-device keyboard includes pinyin, romaji and jamo input.'],
  ['What happens if I forget my password?', 'Six wrong attempts wipe the keys on the device, with a warning before the last one. Your money is untouched on the venue; link the account again with a fresh key.'],
  ['How do I turn it off?', 'Short-press the power key and the screen goes dark while the room keeps playing. Hold it two seconds and a sheet appears with slide to power off and Cancel. Only sliding the slider with your finger powers it down; no key can confirm it.'],
  ['What is in the box?', 'The device and a USB-C cable. Genesis shells are 3D-printed PA12 nylon and numbered 1 to 1,000.'],
];

export default function FaqPage() {
  return (
    <>
      <PageHero eyebrow="FAQ" title={<>Asked.<br /><span className="neon">Answered.</span></>} lead="Short answers. Longer ones live on the product pages." ctas={<PreorderButtons />} />
      <section className="sec" aria-label="Questions">
        <div className="wrap">
          <div className="faq rv">
            {FAQ.map(([q, a]) => (
              <details key={q}>
                <summary>{q}</summary>
                <div className="answer"><p>{a}</p></div>
              </details>
            ))}
          </div>
        </div>
      </section>
    </>
  );
}
