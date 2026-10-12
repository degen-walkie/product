// Demo mode: scripted walkthroughs played on the real firmware against the mocks.
// Each step drives the device the way a person would (wheel, trade key, power key,
// taps, swipes, the mic) and the page lights up whatever is being pressed.
import { IN } from './host.js';

/* Screen coordinates are in device pixels (320 x 480). */
export const DEMOS = [
  {
    id: 'power', title: 'Power on', blurb: 'The device starts powered off. Hold the power key, watch it boot into the rooms.', fresh: false, off: true,
    steps: [
      ['cap', 'The device is off.'], ['wait', 1200],
      ['cap', 'Hold the power key for two seconds.'], ['holdPower'], ['wait', 2600],
      ['cap', 'Rooms: one live room per ticker. Prices are live.'], ['wait', 1800],
      ['cap', 'Roll the thumb wheel to move.'], ['roll', 1, 2], ['wait', 600], ['roll', -1, 2], ['wait', 900],
      ['cap', 'Swipe between rooms, positions, markets and news.'], ['swipe', 260, 260, 60, 260], ['wait', 1600], ['swipe', 60, 260, 260, 260], ['wait', 1200],
    ],
  },
  {
    id: 'setup', title: 'First-time setup', blurb: 'What you see the first time you turn it on: language, Wi-Fi, a PIN, and linking your first venue.', fresh: true, off: true,
    steps: [
      ['cap', 'A new device, straight out of the box. Hold power.'], ['holdPower'], ['wait', 2600],
      ['cap', 'Pick the language you want to hear the rooms in.'], ['wait', 1200], ['roll', 1, 1], ['wait', 500], ['roll', -1, 1], ['wait', 500],
      ['cap', 'The trade key selects.'], ['trade'], ['wait', 1300],
      ['cap', 'It joins Wi-Fi. A 4G back-clip works too.'], ['wait', 1300], ['trade'], ['wait', 1100],
      ['cap', 'Set a six-digit PIN: roll for the digit, trade key for the next. Or tap.'],
      ['roll', 1, 4], ['trade'], ['roll', 1, 2], ['trade'], ['tap', 261, 308, '9'], ['tap', 58, 308, '7'], ['tap', 261, 352, '0'], ['tap', 160, 220, '2'], ['wait', 1200],
      ['cap', 'Link a venue. The device makes its own trading key.'], ['wait', 1200], ['trade'], ['wait', 1600],
      ['cap', 'Scan the QR with your phone and approve it in your main wallet.'], ['wait', 1600], ['approve'], ['wait', 4200],
      ['cap', 'Linked. The key can trade. It cannot withdraw.'], ['wait', 1200], ['roll', 1, 3], ['trade'], ['wait', 1300],
      ['cap', 'Done. The trade key starts.'], ['wait', 1600], ['trade'], ['wait', 1600],
    ],
  },
  {
    id: 'open', title: 'Open a position', blurb: 'From a live room to a signed long in a few taps. Only the trade key signs.',
    steps: [
      ['cap', 'Open the #SOL room with the trade key.'], ['trade'], ['wait', 1500],
      ['cap', 'In a room, the trade key opens a ticket for that market.'], ['trade'], ['wait', 1200],
      ['cap', 'Tap 10x leverage.'], ['tap', 158, 172, '10x'], ['wait', 700],
      ['cap', 'Size: half of your free margin.'], ['tap', 130, 232, '50%'], ['wait', 900],
      ['cap', 'Review.'], ['tap', 160, 428, 'Review'], ['wait', 1300],
      ['cap', 'The confirmation sheet. Touch can\'t sign: press the trade key.'], ['wait', 1400], ['trade'], ['wait', 900],
      ['cap', 'Blue light: signed on the device, sent to Hyperliquid.'], ['wait', 1600],
      ['cap', 'Filled. Green light for three seconds.'], ['wait', 2600],
      ['cap', 'The Orders tab shows the position.'], ['roll', 1, 2], ['wait', 2200],
    ],
  },
  {
    id: 'close', title: 'Close a position', blurb: 'Find a position, close half of it or all of it.',
    steps: [
      ['cap', 'Swipe to Positions.'], ['swipe', 260, 280, 60, 280], ['wait', 1600],
      ['cap', 'Each card shows PnL and how far it is from liquidation.'], ['wait', 1600],
      ['cap', 'Open SOL.'], ['roll', 1, 1], ['wait', 400], ['trade'], ['wait', 1600],
      ['cap', 'Live candles, your entry line and the position.'], ['wait', 1600],
      ['cap', 'Roll to CLOSE.'], ['roll', 1, 2], ['wait', 600], ['trade'], ['wait', 1300],
      ['cap', 'Reduce-only, at market. Press the trade key to sign.'], ['wait', 1200], ['trade'], ['wait', 3600],
      ['cap', 'Closed. The PnL is now in your balance.'], ['wait', 1600],
    ],
  },
  {
    id: 'limit', title: 'Limit order', blurb: 'Place a limit below the market, see it rest, then cancel it.',
    steps: [
      ['cap', 'Open #SOL and its ticket.'], ['trade'], ['wait', 1400], ['trade'], ['wait', 1100],
      ['cap', 'Switch to Limit.'], ['tap', 269, 112, 'Limit'], ['wait', 900],
      ['cap', 'Tap the limit price, then roll the wheel to move it.'], ['tap', 160, 276, 'Price'], ['wait', 500], ['roll', -1, 10], ['wait', 400],
      ['cap', 'Trade key: done.'], ['trade'], ['wait', 600],
      ['cap', 'Roll down to Review, then sign.'], ['roll', 1, 3], ['wait', 500], ['trade'], ['wait', 1300], ['trade'], ['wait', 3300],
      ['cap', 'It rests on the book. Orders tab:'], ['roll', 1, 2], ['wait', 1800],
      ['cap', 'Changed your mind? Trade key → cancel.'], ['trade'], ['wait', 1300], ['trade'], ['wait', 2600],
    ],
  },
  {
    id: 'tpsl', title: 'Take profit / stop loss', blurb: 'Open a long with a +5% take profit and a −3% stop, both set on the venue.',
    steps: [
      ['cap', 'Open the #SOL ticket.'], ['trade'], ['wait', 1400], ['trade'], ['wait', 1100],
      ['cap', 'Take profit +5%.'], ['tap', 94, 288, '+5%'], ['wait', 700],
      ['cap', 'Stop loss −3%.'], ['tap', 244, 288, '-3%'], ['wait', 900],
      ['cap', 'The ticket shows the trigger prices.'], ['wait', 1300],
      ['cap', 'Review and sign once.'], ['tap', 160, 428, 'Review'], ['wait', 1400], ['trade'], ['wait', 4600],
      ['cap', 'Filled, then TP and SL placed as trigger orders.'], ['wait', 1200],
      ['cap', 'Orders tab: TP and SL wait on the venue.'], ['roll', 1, 2], ['wait', 2600],
    ],
  },
  {
    id: 'room', title: 'Live room + voice', blurb: 'Hold to talk in a room. Everyone hears you in their own language.',
    steps: [
      ['cap', 'Open #SOL.'], ['trade'], ['wait', 1800],
      ['cap', 'Messages arrive translated: JA → EN, ES → EN…'], ['wait', 2500],
      ['cap', 'Hold the mic and talk.'], ['say', 'Rolling into HYPE after this candle.'], ['wait', 2600],
      ['cap', 'Someone answers.'], ['wait', 3600],
      ['cap', 'Roll for the chart, without leaving the room.'], ['roll', 1, 1], ['wait', 2600],
    ],
  },
  {
    id: 'ask', title: 'Ask the AI', blurb: 'Ask about your own book. The AI reads; it can\'t trade.',
    steps: [
      ['cap', 'Swipe to Ask.'], ['swipe', 60, 280, 260, 280], ['wait', 900], ['swipe', 60, 280, 260, 280], ['wait', 1400],
      ['cap', 'Pick a question with the wheel, or hold the mic.'], ['roll', 1, 1], ['wait', 500], ['roll', -1, 1], ['trade'], ['wait', 3200],
      ['cap', 'The answer uses your live positions.'], ['wait', 2200],
      ['cap', 'Or ask by voice.'], ['say', 'What is funding doing on my positions?'], ['wait', 3600],
    ],
  },
  {
    id: 'switch', title: 'Assets & venues', blurb: 'Hyperliquid, Pacifica and Lighter side by side, one balance view.',
    steps: [
      ['cap', 'Swipe to Markets.'], ['swipe', 260, 280, 60, 280], ['wait', 800], ['swipe', 260, 280, 60, 280], ['wait', 1600],
      ['cap', 'Perps on three venues. Roll down to Pacifica.'], ['roll', 1, 5], ['wait', 1200],
      ['cap', 'Open ETH on Pacifica.'], ['trade'], ['wait', 2200],
      ['cap', 'Same screens on every venue.'], ['wait', 1200], ['back'], ['wait', 800],
      ['cap', 'Accounts: one card per venue, funds stay on each.'], ['swipe', 260, 280, 60, 280], ['wait', 900], ['swipe', 260, 280, 60, 280], ['wait', 900], ['swipe', 260, 280, 60, 280], ['wait', 2800],
    ],
  },
  {
    id: 'voiceorder', title: 'Voice order', blurb: 'Say the order. It becomes a ticket. The trade key still signs.',
    steps: [
      ['cap', 'From any screen, hold the mic and say the order.'], ['say', 'Long SOL five x five hundred USDC'], ['wait', 1800],
      ['cap', 'Parsed into a ticket. Nothing is sent yet.'], ['wait', 1800],
      ['cap', 'Trade key: review. Trade key again: sign.'], ['trade'], ['wait', 1300], ['trade'], ['wait', 3800],
    ],
  },
  {
    id: 'screen', title: 'Screen, quick panel, power off', blurb: 'Pull down for quick settings, radio mode, and the slide-to-power-off sheet.',
    steps: [
      ['cap', 'Pull down from the top.'], ['swipe', 160, 50, 160, 300], ['wait', 1400],
      ['cap', 'Quick settings. Radio mode turns the screen off and keeps the room playing.'], ['wait', 1300], ['trade'], ['wait', 1800],
      ['cap', 'Press power to wake it.'], ['power'], ['wait', 1400],
      ['cap', 'Hold power for the power-off sheet.'], ['holdPower'], ['wait', 1600],
      ['cap', 'Slide, or press the trade key.'], ['trade'], ['wait', 2400],
      ['cap', 'Off. Hold power to start again.'], ['wait', 1200],
    ],
  },
];

/* ---------------- player ---------------- */

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

export function createPlayer({ host, fx, onCaption, onStep, approve, setMic }) {
  let stop = false;

  const KEY = {
    roll: (d) => (d > 0 ? IN.DOWN : IN.UP),
  };

  async function touchPath(points, ms) {
    const [x0, y0] = points[0];
    host.touch(x0, y0, true);
    const steps = 12;
    for (let i = 1; i <= steps; i++) {
      const t = i / steps;
      const x = Math.round(x0 + (points[1][0] - x0) * t), y = Math.round(y0 + (points[1][1] - y0) * t);
      host.touch(x, y, true);
      await sleep(ms / steps);
    }
    host.touch(points[1][0], points[1][1], false);
  }

  const run = {
    async cap(t) { onCaption(t); },
    async wait(ms) { await sleep(ms); },
    async roll(dir, n) {
      for (let i = 0; i < n && !stop; i++) {
        fx.key('wheel', dir > 0 ? 'Roll ▼' : 'Roll ▲');
        host.input(KEY.roll(dir));
        await sleep(330);
      }
    },
    async back() { fx.key('wheel', 'Press wheel'); host.input(IN.PRESS); await sleep(400); },
    async trade() { fx.key('trade', 'Trade key'); host.input(IN.TRADE); await sleep(450); },
    async power() { fx.key('power', 'Power'); host.input(IN.POWER); await sleep(400); },
    async holdPower() { fx.key('power', 'Hold 2 s', 1700); await sleep(1700); host.input(IN.POWER_HOLD); },
    async tap(x, y, label) {
      fx.tap(x, y, label);
      host.touch(x, y, true);
      await sleep(90);
      host.touch(x, y, false);
      await sleep(350);
    },
    async swipe(x1, y1, x2, y2) {
      fx.swipe(x1, y1, x2, y2);
      await touchPath([[x1, y1], [x2, y2]], 220);
      await sleep(350);
    },
    async say(text) {
      setMic(text);
      fx.key('ptt', `“${text}”`, 1500);
      host.input(IN.PTT_DOWN);
      await sleep(1500);
      host.input(IN.PTT_UP);
      await sleep(500);
    },
    async approve() { fx.phone(); await approve(); },
  };

  return {
    async play(demo) {
      stop = false;
      for (let i = 0; i < demo.steps.length && !stop; i++) {
        const [op, ...args] = demo.steps[i];
        onStep(i);
        await run[op](...args);
      }
      onStep(-1);
      return !stop;
    },
    stop() { stop = true; },
  };
}
