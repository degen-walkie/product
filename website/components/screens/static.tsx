'use client';

import { Bar, Chart, Hint, Icon, Kbd, Msg, Ptt, Row, Tabs } from './parts';

/* Static screens (one focus each, 3-slot hint bar on every one).
   Model: roll = move / adjust / page · press = back (menu at top level) · trade = open / enter / SIGN. */

export function RoomsScreen({ focus = 0 }: { focus?: number }) {
  const rooms = [
    ['#SOL', <>Solana · 212.40 · <span style={{ color: '#14f195' }}>+3.1%</span></>, '1,204'],
    ['#HYPE', <>Hyperliquid · 24.18 · <span style={{ color: '#14f195' }}>+6.4%</span></>, '688'],
    ['#BTC', <>Lighter · 118,240 · <span style={{ color: '#ff4d4d' }}>-0.8%</span></>, '2,341'],
    ['#ETH', 'Pacifica · 4,412.50', '1,019'],
    ['#4471', 'Private · your crew', '6'],
  ] as const;
  return (
    <>
      <Bar left={<span className="room">ROOMS</span>} />
      <div className="ui-body">
        <div className="ui-title">Rooms</div>
        <div className="ui-sub">ROLL TO PICK · TRADE KEY TO JOIN</div>
        <div className="ui-list">
          {rooms.map((r, i) => (
            <Row key={r[0]} title={r[0]} sub={r[1]} val={r[2]} small="listening" sel={i === focus} />
          ))}
        </div>
        <div className="ui-btn">+ Join by number</div>
      </div>
      <Hint roll="move" press="menu" trade="open room" />
    </>
  );
}

/* Room view. Pages: Chat | Chart | Orders. Default focus = the ticker header, so the trade key opens the SOL ticket. */
export function RoomScreen({ page = 0 }: { page?: 0 | 1 | 2 }) {
  return (
    <>
      <Bar left={<><span className="room">#SOL</span> <span>· 1,204</span></>} />
      <div className="ui-body tight">
        <div className="ui-strip ui-focus" style={{ borderColor: 'transparent' }}>
          <span style={{ fontFamily: 'var(--display)', fontStretch: '72%', fontWeight: 900, fontSize: '1.5em' }}>ROOM #SOL</span>
          <span>212.40</span>
          <span className="up">+3.1%</span>
        </div>
        <Tabs items={['Chat', 'Chart', 'Orders']} on={page} />
        {page === 0 ? (
          <>
            <div className="ui-feed">
              <Msg who="@kaito" lang="JA → EN" text="Volume on the Base pool just doubled. Anyone else seeing it?" />
              <Msg who="@mara" lang="ES → EN" text="Same on Hyperliquid. Funding just flipped." />
              <Msg who="@you" lang="EN" text="Rolling to #HYPE after this." me />
            </div>
            <div className="ui-toggle">
              <span>
                Radio mode <span className="ui-eyebrow">screen off · keep listening</span>
              </span>
              <span className="ui-sw" />
            </div>
            <Ptt />
          </>
        ) : page === 1 ? (
          <>
            <div className="ui-big">212.40<small>USDC</small></div>
            <div className="ui-sub up">+3.1% · 24 H · 1,204 LISTENING</div>
            <Chart tall />
            <div className="ui-chips"><span className="ui-chip">1H</span><span className="ui-chip on">24H</span><span className="ui-chip">7D</span><span className="ui-chip">30D</span></div>
            <Ptt />
          </>
        ) : (
          <>
            <div className="ui-eyebrow">Your open orders · SOL</div>
            <div className="ui-list">
              <Row title="Buy 1.176 SOL" sub="Limit 210.00 · Hyperliquid" val="250 USDC" small="open" md />
              <Row title="Sell 0.5 SOL" sub="Limit 230.00 · Hyperliquid" val="115 USDC" small="open" md />
            </div>
            <div className="ui-eyebrow c">Filled orders light the status light green for 3 s</div>
            <Ptt />
          </>
        )}
      </div>
      <Hint roll="page" press="back" trade="open SOL" />
    </>
  );
}

export function DiscoverFiltersScreen() {
  return (
    <>
      <Bar left={<span className="room">DISCOVER</span>} />
      <div className="ui-body">
        <div className="ui-title">Discover</div>
        <div className="ui-sub">YOUR THRESHOLDS · NO RANKING</div>
        <div className="ui-list">
          <Row title="Liquidity" sub="minimum" val="≥ $50k" sel mono={false} chev />
          <Row title="Holders" sub="minimum" val="≥ 500" mono={false} chev />
          <Row title="Age" sub="maximum" val="≤ 30 d" mono={false} chev />
          <Row title="Market cap" sub="range" val="$1M – $50M" mono={false} chev />
        </div>
        <div className="ui-eyebrow">Chains</div>
        <div className="ui-chips"><span className="ui-chip on">SOL</span><span className="ui-chip on">BASE</span><span className="ui-chip on">ARB</span><span className="ui-chip">ETH</span><span className="ui-chip">HL</span></div>
        <div className="ui-btn primary">Show 23 matches</div>
      </div>
      <Hint roll="adjust $50k" press="back" trade="set" />
    </>
  );
}

export function DiscoverResultsScreen() {
  return (
    <>
      <Bar left={<><span className="room">DISCOVER</span> <span>· 23</span></>} />
      <div className="ui-body">
        <div className="ui-title">AAPL</div>
        <div className="ui-sub">SAME TICKER · 3 CHAINS · YOU PICK</div>
        <div className="ui-list">
          <Row title="Solana" sub="XsAP…mple1" val="$1.2M" small="liq · 4,811 holders" sel mono={false} md />
          <Row title="Base" sub="0x3a9F…mple2" val="$410k" small="liq · 1,207 holders" mono={false} md />
          <Row title="Arbitrum" sub="0x71cB…mple3" val="$96k" small="liq · 612 holders" mono={false} md />
        </div>
        <div className="ui-card row"><span className="ui-eyebrow">Sorted by</span><span className="ui-badge">Liquidity ↓</span></div>
        <div className="ui-eyebrow c">We list. We don&apos;t recommend.</div>
      </div>
      <Hint roll="move" press="back" trade="open" />
    </>
  );
}

/* Asset page. Pages: Chart | Trade | Holders. Focus on the header → trade opens the HYPE ticket. */
export function AssetScreen() {
  return (
    <>
      <Bar left={<><span className="room">#HYPE</span> <span>· 688</span></>} />
      <div className="ui-body">
        <div className="ui-head ui-focus" style={{ padding: '0.5em 0.7em' }}>
          <div>
            <div className="ui-title">HYPE</div>
            <div className="ui-sub">HYPERLIQUID · SPOT</div>
          </div>
          <span className="ui-badge ok">whitelisted</span>
        </div>
        <Tabs items={['Chart', 'Trade', 'Holders']} on={0} />
        <div className="ui-big">24.18<small>USDC</small></div>
        <div className="ui-sub up">+6.4% · 24 H</div>
        <Chart />
        <div className="ui-chips"><span className="ui-chip">1H</span><span className="ui-chip on">24H</span><span className="ui-chip">7D</span><span className="ui-chip">30D</span></div>
        <div className="ui-eyebrow c">Trade key opens the HYPE ticket</div>
      </div>
      <Hint roll="page" press="back" trade="buy HYPE" />
    </>
  );
}

export function PerpsScreen() {
  return (
    <>
      <Bar left={<span className="room">HYPE-PERP</span>} />
      <div className="ui-body">
        <div className="ui-head">
          <div>
            <div className="ui-title sm">Long HYPE</div>
            <div className="ui-sub">PERPETUAL · HYPERLIQUID</div>
          </div>
          <span className="ui-badge hot">perps · eligible</span>
        </div>
        <div className="ui-seg"><span className="on">Long</span><span>Short</span><span>Market</span><span className="on">Limit</span></div>
        <div className="ui-eyebrow">Leverage</div>
        <div className="ui-lev ui-focus" style={{ padding: '0.3em' }}><span>2×</span><span>3×</span><span className="on">5×</span><span>10×</span><span>20×</span></div>
        <div className="ui-eyebrow">Size · of free margin</div>
        <div className="ui-chips"><span className="ui-chip">10%</span><span className="ui-chip on">25%</span><span className="ui-chip">50%</span><span className="ui-chip">100%</span></div>
        <dl className="ui-kv">
          <dt>Size</dt><dd>500.00 USDC</dd>
          <dt>Limit</dt><dd>24.10</dd>
          <dt>TP / SL</dt><dd><span className="neon">27.50</span> / <span className="bad">22.80</span></dd>
          <dt>Liq. price</dt><dd className="bad">19.84</dd>
        </dl>
        <div className="ui-btn trade"><Icon name="key" /> Review → send</div>
      </div>
      <Hint roll="adjust" press="back" trade="review" />
    </>
  );
}

/* Linked accounts. Money stays on each venue; the device holds trade-only keys. */
export function AccountsScreen() {
  return (
    <>
      <Bar left={<span className="room">ACCOUNTS</span>} />
      <div className="ui-body">
        <div className="ui-title">Accounts</div>
        <div className="ui-sub">FUNDS STAY ON THE VENUE</div>
        <div className="ui-card neon ui-focus">
          <div style={{ display: 'flex', justifyContent: 'space-between' }}><span className="ui-eyebrow" style={{ color: '#14f195' }}>Hyperliquid</span><span className="ui-badge ok">trade-only</span></div>
          <div className="ui-big md">4,210.55<small>USDC</small></div>
          <div className="ui-sub" style={{ marginTop: '0.2em' }}>PERPS + SPOT · AGENT 0x8f3…c21</div>
          <div className="ui-eyebrow" style={{ marginTop: '0.6em' }}>Can trade · can&apos;t withdraw</div>
        </div>
        <div className="ui-card">
          <div style={{ display: 'flex', justifyContent: 'space-between' }}><span className="ui-eyebrow" style={{ color: '#14f195' }}>Pacifica</span><span className="ui-badge ok">solana perps</span></div>
          <div className="ui-big md">1,350.00<small>USDC</small></div>
          <div className="ui-sub" style={{ marginTop: '0.2em' }}>PERPS · AGENT 7Fq2…mP9a</div>
        </div>
        <div className="ui-btns"><div className="ui-btn"><Icon name="qr" /> Link</div><div className="ui-btn bad"><Icon name="x" /> Revoke</div></div>
      </div>
      <Hint roll="move" press="menu" trade="open" />
    </>
  );
}

/* Linking Hyperliquid: approve an agent key from your main wallet in the browser, scan, done. */
export function LinkScreen() {
  return (
    <>
      <Bar left={<><span className="room">LINK</span> <span>· HYPERLIQUID</span></>} />
      <div className="ui-body">
        <div className="ui-title sm">Link Hyperliquid</div>
        <div className="ui-sub">APPROVED IN YOUR MAIN WALLET</div>
        <dl className="ui-kv">
          <dt>Agent</dt><dd>0x8f3…c21</dd>
          <dt>Trade</dt><dd className="neon">allowed</dd>
          <dt>Withdraw</dt><dd className="bad">impossible</dd>
          <dt>Change permissions</dt><dd className="bad">impossible</dd>
          <dt>Revoke</dt><dd>any time, from your main wallet</dd>
        </dl>
        <div className="ui-card ui-focus">
          <div className="ui-eyebrow"><Icon name="lock" style={{ width: '1.2em', height: '1.2em', display: 'inline-block', verticalAlign: '-0.2em' }} /> Device password</div>
          <div className="ui-big" style={{ fontSize: '2em', letterSpacing: '0.3em', marginTop: '0.4em' }}>••••••<span className="ui-caret" /></div>
        </div>
        <Kbd keys="1234567890" on="7" />
        <div className="ui-eyebrow c">The key is stored encrypted on the device</div>
      </div>
      <Hint roll="digit" press="back" trade="enter" />
    </>
  );
}

/* Lost it? Revoke it. Shown from the device; the same works from the venue without it. */
export function RevokeScreen() {
  return (
    <>
      <Bar left={<><span className="room">ACCOUNTS</span> <span>· REVOKE</span></>} />
      <div className="ui-body">
        <div className="ui-title sm">Revoke access</div>
        <div className="ui-sub">YOUR FUNDS DON&apos;T MOVE</div>
        <div className="ui-list">
          <Row title="Hyperliquid" sub="agent 0x8f3…c21" val="revoke" small="trade-only" sel mono={false} md />
          <Row title="Pacifica" sub="agent 7Fq2…mP9a" val="revoke" small="solana perps" mono={false} md />
          <Row title="Lighter" sub="API key #3" val="revoke" small="robinhood chain" mono={false} md />
        </div>
        <div className="ui-card warn">
          <div className="ui-eyebrow" style={{ color: '#ffb020' }}>Lost the device?</div>
          <div style={{ fontSize: '1.25em', marginTop: '0.3em' }}>Revoke from your main wallet or the venue. Your funds stay on the venue either way.</div>
        </div>
        <div className="ui-sign"><span className="kg" /> Press the trade key to revoke</div>
      </div>
      <Hint roll="move" press="back" trade="revoke" sign />
    </>
  );
}

/* Meme order through GMGN's trading API. Funds sit in your GMGN account. */
export function MemeScreen() {
  return (
    <>
      <Bar left={<><span className="room">#WIF</span> <span>· MEME</span></>} />
      <div className="ui-body">
        <div className="ui-title sm">Buy WIF</div>
        <div className="ui-sub">SOLANA · VIA GMGN</div>
        <div className="ui-card ui-focus">
          <div className="ui-eyebrow">You pay</div>
          <div className="ui-big" style={{ fontSize: '2.4em', marginTop: '0.3em' }}>0.50<small>SOL</small></div>
        </div>
        <dl className="ui-kv">
          <dt>Receive</dt><dd className="neon">≈ 61.2 WIF</dd>
          <dt>Account</dt><dd>your GMGN account</dd>
          <dt>Liquidity</dt><dd>$4.8M</dd>
          <dt>Holders</dt><dd>212,400</dd>
        </dl>
        <div className="ui-eyebrow">Slippage</div>
        <div className="ui-chips"><span className="ui-chip">1%</span><span className="ui-chip on">3%</span><span className="ui-chip">5%</span><span className="ui-chip">Custom</span></div>
        <div className="ui-btn trade"><Icon name="key" /> Review</div>
      </div>
      <Hint roll="amount" press="back" trade="review" />
    </>
  );
}

export function SettingsScreen() {
  return (
    <>
      <Bar left={<span className="room">SETTINGS</span>} />
      <div className="ui-body">
        <div className="ui-title sm">Language</div>
        <div className="ui-sub">HEAR EVERY ROOM IN YOUR LANGUAGE</div>
        <div className="ui-list">
          <Row title="English" sub="EN · QWERTY" sel mono={false} md ok />
          <Row title={<span lang="zh">中文</span>} sub="ZH · Pinyin keyboard" mono={false} md />
          <Row title={<span lang="ja">日本語</span>} sub="JA · Romaji keyboard" mono={false} md />
          <Row title={<span lang="ko">한국어</span>} sub="KO · Jamo keyboard" mono={false} md />
          <Row title={<span lang="ar">العربية</span>} sub="AR" mono={false} md />
          <Row title="Español" sub="ES" mono={false} md />
        </div>
        <div className="ui-chips"><span className="ui-chip">PT</span><span className="ui-chip">DE</span><span className="ui-chip">FR</span><span className="ui-chip">PL</span><span className="ui-chip">RU</span><span className="ui-chip">IT</span></div>
      </div>
      <Hint roll="move" press="back" trade="select" />
    </>
  );
}

/* AI proposal: the agent's queued swap shown as a confirmation. Only the trade key signs it. */
export function AgentScreen() {
  return (
    <>
      <Bar left={<><span className="room hot">AGENT</span> <span>· MCP</span></>} />
      <div className="ui-body">
        <div className="ui-head">
          <div>
            <div className="ui-title sm">Proposal waiting</div>
            <div className="ui-sub">PURPLE LIGHT · YOUR CALL</div>
          </div>
          <span className="ui-badge hot">1 pending</span>
        </div>
        <div className="ui-card hot ui-focus" style={{ boxShadow: 'inset 0 0 0 0.2em #9945ff' }}>
          <div style={{ display: 'flex', gap: '0.8em', alignItems: 'center', color: '#c89bff' }}><Icon name="bot" style={{ width: '1.4em', height: '1.4em' }} /><span className="ui-eyebrow" style={{ color: '#c89bff' }}>Agent proposes</span></div>
          <div style={{ fontSize: '1.9em', fontWeight: 700, marginTop: '0.5em', fontFamily: 'var(--mono)' }}>Sell 14 HYPE → USDC</div>
          <div className="ui-sub" style={{ marginTop: '0.3em' }}>HYPERLIQUID SPOT · TRADE-ONLY KEY</div>
        </div>
        <dl className="ui-kv">
          <dt>Agent note</dt><dd className="sans">“Take profit at 212.”</dd>
          <dt>Expires</dt><dd>in 4 min</dd>
          <dt>Can confirm</dt><dd className="neon">Only you</dd>
        </dl>
        <div className="ui-sign"><span className="kg" /> Press the trade key to sign</div>
        <div className="ui-btn bad"><Icon name="x" /> Dismiss (press)</div>
        <div className="ui-eyebrow c">Agents read &amp; propose. Your press signs.</div>
      </div>
      <Hint roll="details" press="dismiss" trade="sign" sign />
    </>
  );
}

export function ChargingScreen() {
  return (
    <>
      <Bar left={<span className="room warn">CHARGING</span>} bat={72} charging />
      <div className="ui-body">
        <div className="ui-big">72%<small>≈ 40 min to full</small></div>
        <div className="ui-prog"><i style={{ width: '72%', background: '#ffb020' }} /></div>
        <div className="ui-eyebrow">Status light</div>
        <div className="ui-legend">
          <div style={{ color: '#4da3ff' }}><i /><span><b>Blue blink</b> — sending · awaiting confirmation</span></div>
          <div style={{ color: '#14f195' }}><i /><span><b>Green 3 s</b> — confirmed</span></div>
          <div style={{ color: '#ff4d4d' }}><i /><span><b>Red ×3</b> — failed</span></div>
          <div style={{ color: '#ff4d4d' }}><i /><span><b>Red every 5 s</b> — battery under 10%</span></div>
          <div style={{ color: '#ffb020' }}><i /><span><b>Yellow</b> — charging</span></div>
          <div style={{ color: '#14f195' }}><i /><span><b>Green</b> — full</span></div>
          <div style={{ color: '#9945ff' }}><i /><span><b>Purple slow</b> — AI proposal waiting</span></div>
        </div>
      </div>
      <Hint roll={null} press="menu" trade={null} />
    </>
  );
}

/* Spot ticket (before the confirmation sheet). Focus on the amount → roll adjusts it. */
export function SpotTicket({ focusAmount = true, amount = '250.00' }: { focusAmount?: boolean; amount?: string }) {
  return (
    <>
      <Bar left={<><span className="room">#HYPE</span> <span>· ORDER</span></>} />
      <div className="ui-body">
        <div className="ui-title sm">Buy HYPE</div>
        <div className="ui-sub">SPOT · HYPERLIQUID</div>
        <div className={`ui-card${focusAmount ? ' ui-focus' : ''}`}>
          <div className="ui-eyebrow">You pay</div>
          <div className="ui-big" style={{ fontSize: '2.4em', marginTop: '0.3em' }}>{amount}<small>USDC</small></div>
        </div>
        <dl className="ui-kv">
          <dt>Receive</dt><dd className="neon">≈ 10.34 HYPE</dd>
          <dt>Account</dt><dd>Hyperliquid</dd>
          <dt>Fee</dt><dd>≈ 0.09 USDC</dd>
          <dt>Price impact</dt><dd>0.02%</dd>
        </dl>
        <div className="ui-eyebrow">Slippage</div>
        <div className="ui-chips"><span className="ui-chip">0.1%</span><span className="ui-chip on">0.5%</span><span className="ui-chip">1.0%</span><span className="ui-chip">Custom</span></div>
        <div className={`ui-btn trade${focusAmount ? '' : ' ui-focus'}`}><Icon name="key" /> Review</div>
      </div>
      <Hint roll="amount" press="back" trade="review" />
    </>
  );
}

/* Voice order: hold to speak, the device parses it into a ticket. Nothing is sent until the trade key. */
export function VoiceScreen() {
  return (
    <>
      <Bar left={<><span className="room">#SOL</span> <span>· VOICE</span></>} />
      <div className="ui-body">
        <div className="ui-eyebrow">You said</div>
        <div className="ui-card" style={{ fontSize: '1.45em', lineHeight: 1.35 }}>&ldquo;Long SOL, five x, five hundred USDC.&rdquo;</div>
        <div className="ui-eyebrow">Ticket</div>
        <dl className="ui-kv ui-focus" style={{ padding: '0.5em 0.7em' }}>
          <dt>Side</dt><dd className="neon">LONG</dd>
          <dt>Market</dt><dd>SOL-PERP · Hyperliquid</dd>
          <dt>Leverage</dt><dd>5×</dd>
          <dt>Size</dt><dd>500.00 USDC</dd>
          <dt>Liq. price</dt><dd className="bad">171.20</dd>
        </dl>
        <div className="ui-sign"><span className="kg" /> Press the trade key to send</div>
        <Ptt label="HOLD" sub="TO SPEAK" />
      </div>
      <Hint roll="edit" press="cancel" trade="send" sign />
    </>
  );
}

/* Ask the AI: same mic, answers about your own positions and the market. Read-only. */
export function AskScreen() {
  return (
    <>
      <Bar left={<><span className="room hot">ASK</span> <span>· AI</span></>} />
      <div className="ui-body">
        <div className="ui-eyebrow">You asked</div>
        <div className="ui-card" style={{ fontSize: '1.3em' }}>&ldquo;How far is my SOL long from liquidation?&rdquo;</div>
        <div className="ui-card hot">
          <div style={{ display: 'flex', gap: '0.8em', alignItems: 'center', color: '#c89bff' }}><Icon name="bot" style={{ width: '1.4em', height: '1.4em' }} /><span className="ui-eyebrow" style={{ color: '#c89bff' }}>Answer</span></div>
          <div style={{ fontSize: '1.3em', marginTop: '0.4em', lineHeight: 1.4 }}>SOL is 212.40. Your liquidation is 171.20, 19.4% away. Funding is paying you 0.01% an hour.</div>
        </div>
        <div className="ui-btns"><div className="ui-btn"><Icon name="chart" /> Open position</div><div className="ui-btn hot"><Icon name="bot" /> Ask again</div></div>
        <Ptt label="HOLD" sub="TO ASK" />
      </div>
      <Hint roll="scroll" press="back" trade="open" />
    </>
  );
}
