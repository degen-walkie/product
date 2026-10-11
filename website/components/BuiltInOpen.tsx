import { GITHUB_URL, X_URL } from '@/lib/site';

/* "Built in the open" — hidden entirely when neither link is configured. */
export default function BuiltInOpen() {
  if (!GITHUB_URL && !X_URL) return null;
  return (
    <section className="sec alt" aria-labelledby="open-title">
      <div className="wrap">
        <div className="split v-center">
          <div className="rv">
            <p className="eyebrow">Built in the open</p>
            <h2 id="open-title" className="display">Watch it<br />get <span className="neon">made.</span></h2>
            <p className="lead mt-18">Firmware, schematic, enclosure CAD and this site. Follow the commits, file an issue, or just heckle.</p>
            <div className="open-links mt-22">
              {GITHUB_URL ? (
                <a href={GITHUB_URL} target="_blank" rel="noopener noreferrer">
                  <svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M12 .5a12 12 0 0 0-3.8 23.4c.6.1.8-.3.8-.6v-2.2c-3.3.7-4-1.4-4-1.4-.5-1.4-1.3-1.8-1.3-1.8-1.1-.7.1-.7.1-.7 1.2.1 1.8 1.2 1.8 1.2 1.1 1.8 2.8 1.3 3.5 1 .1-.8.4-1.3.8-1.6-2.7-.3-5.5-1.3-5.5-5.9 0-1.3.5-2.4 1.2-3.2-.1-.3-.5-1.5.1-3.2 0 0 1-.3 3.3 1.2a11.5 11.5 0 0 1 6 0c2.3-1.5 3.3-1.2 3.3-1.2.6 1.7.2 2.9.1 3.2.8.8 1.2 1.9 1.2 3.2 0 4.6-2.8 5.6-5.5 5.9.4.4.8 1.1.8 2.2v3.3c0 .3.2.7.8.6A12 12 0 0 0 12 .5z" /></svg>
                  GitHub
                </a>
              ) : null}
              {X_URL ? (
                <a href={X_URL} target="_blank" rel="noopener noreferrer">
                  <svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M18.2 2h3.4l-7.4 8.5L23 22h-6.8l-5.3-7-6.1 7H1.4l7.9-9.1L1 2h7l4.8 6.4L18.2 2zm-1.2 18h1.9L7.1 3.9H5.1L17 20z" /></svg>
                  X
                </a>
              ) : null}
            </div>
          </div>
          <div className="grid g2 rv d1">
            <div className="card"><div className="idx">Open during the pre-order</div><h3>Firmware + schematic</h3><p>The code that holds your trading keys, the board it runs on, and the MCP interface spec.</p></div>
            <div className="card"><div className="idx">Open when Genesis ships</div><h3>Enclosure CAD</h3><p>Print your own shell. Mod it.</p></div>
          </div>
        </div>
      </div>
    </section>
  );
}
