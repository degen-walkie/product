import Link from 'next/link';
import type { ReactNode } from 'react';
import { LED_LEGEND, PHASE1_REGIONS, ROOMS } from '@/lib/site';

/* Small shared server components: page hero, tapes, LED legend, region list, check box. */

export function PageHero({ eyebrow, title, lead, ctas, hot }: { eyebrow: string; title: ReactNode; lead: string; ctas?: ReactNode; hot?: boolean }) {
  return (
    <section className="phero">
      <div className="wrap">
        <p className={`eyebrow${hot ? ' hot' : ''}`}>{eyebrow}</p>
        <h1 className="display">{title}</h1>
        <p className="lead">{lead}</p>
        {ctas ? <div className="cta-row">{ctas}</div> : null}
      </div>
    </section>
  );
}

export function Tape({ items = ROOMS, className = '' }: { items?: string[]; className?: string }) {
  /* the list is rendered twice so translateX(-50%) loops seamlessly */
  const all = [...items, ...items];
  return (
    <div className={`tape ${className}`} aria-hidden="true">
      <div className="tape-track">
        {all.map((t, i) => (
          <span key={i}>{t}</span>
        ))}
      </div>
    </div>
  );
}

export function LedLegend({ wide }: { wide?: boolean }) {
  return (
    <div className={`leds${wide ? ' wide' : ''}`}>
      {LED_LEGEND.map((l) => (
        <div className={`led ${l.cls}`} key={l.title}>
          <i className={l.anim} />
          <div>
            <b>{l.title}</b>
            <span>{l.text}</span>
          </div>
        </div>
      ))}
    </div>
  );
}

export function Regions() {
  return (
    <ul className="regions">
      {PHASE1_REGIONS.map((r) => (
        <li key={r}>{r}</li>
      ))}
      <li className="more">more regions as approvals land</li>
    </ul>
  );
}

export function CheckBox() {
  return (
    <div className="box" aria-hidden="true">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.6" strokeLinecap="round" strokeLinejoin="round">
        <path d="M4 12l5 5L20 7" />
      </svg>
    </div>
  );
}

export function PreorderButtons({ secondary, secondaryHref }: { secondary?: string; secondaryHref?: string }) {
  return (
    <>
      <Link className="btn btn-neon" href="/preorder">Pre-order — $99 · refundable</Link>
      {secondary && secondaryHref ? (
        <Link className="btn btn-ghost" href={secondaryHref}>{secondary}</Link>
      ) : null}
    </>
  );
}

export function Poster({ title, em, text }: { title: string; em: string; text: string }) {
  return (
    <section className="poster" aria-label="Statement">
      <div className="wrap rv">
        <h2 className="display">
          {title}
          <br />
          <em>{em}</em>
        </h2>
        <p>{text}</p>
      </div>
    </section>
  );
}
