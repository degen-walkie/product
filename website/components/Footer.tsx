import Link from 'next/link';
import { DISCLAIMERS, FINEPRINT } from '@/lib/site';

export default function Footer() {
  return (
    <footer className="footer">
      <div className="wrap">
        <div className="foot-top">
          <div className="foot-brand">
            <Link className="brand" href="/">
              <i aria-hidden="true" />
              Degen Walkie
            </Link>
            <p>Genesis Edition. 1,000 units. $99. A perp trading terminal with a live room for every ticker. We never hold your money. Talk your book, then trade it.</p>
          </div>
          <div>
            <h4>Product</h4>
            <ul>
              <li><Link href="/hardware">Hardware</Link></li>
              <li><Link href="/talk">Rooms</Link></li>
              <li><Link href="/trade">Trade</Link></li>
              <li><Link href="/security">Security</Link></li>
              <li><Link href="/screens">Screens</Link></li>
            </ul>
          </div>
          <div>
            <h4>More</h4>
            <ul>
              <li><Link href="/clip">4G back-clip</Link></li>
              <li><Link href="/roadmap">Roadmap</Link></li>
              <li><Link href="/faq">FAQ</Link></li>
              <li><Link href="/preorder">Pre-order</Link></li>
            </ul>
          </div>
          <div>
            <h4>Legal</h4>
            <ul>
              <li><Link href="/refund-policy">Refund policy</Link></li>
              <li><Link href="/privacy">Privacy</Link></li>
              <li><Link href="/terms">Terms</Link></li>
            </ul>
          </div>
        </div>
        <div className="disclaimers">
          {DISCLAIMERS.map((d) => (
            <p key={d}>{d}</p>
          ))}
        </div>
        <div className="fineprint">
          {FINEPRINT.map((d) => (
            <p key={d.slice(0, 24)}>{d}</p>
          ))}
        </div>
        <div className="foot-bottom">
          <span>© {new Date().getFullYear()} Degen Walkie · Genesis Edition</span>
          <span>
            <Link href="/refund-policy">Refunds</Link> · <Link href="/privacy">Privacy</Link> · <Link href="/terms">Terms</Link> · <a href="#top">Back to top</a>
          </span>
        </div>
      </div>
    </footer>
  );
}
