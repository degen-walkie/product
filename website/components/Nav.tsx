'use client';

import Link from 'next/link';
import { usePathname } from 'next/navigation';
import { useEffect, useState } from 'react';
import { NAV_LINKS } from '@/lib/site';

function isActive(pathname: string, href: string) {
  if (href === '/') return pathname === '/';
  return pathname === href || pathname.startsWith(href + '/');
}

export default function Nav() {
  const pathname = usePathname() || '/';
  const [open, setOpen] = useState(false);
  useEffect(() => setOpen(false), [pathname]);

  const cur = (href: string) => (isActive(pathname, href) ? 'page' : undefined);

  return (
    <header className="nav">
      <div className="wrap">
        <Link className="brand" href="/" aria-label="Degen Walkie home">
          <i aria-hidden="true" />
          Degen Walkie
        </Link>
        <nav className="nav-links" aria-label="Primary">
          {NAV_LINKS.map((l) => (
            <Link key={l.href} href={l.href} aria-current={cur(l.href)}>
              {l.label}
            </Link>
          ))}
        </nav>
        <div className="nav-right">
          <Link className="btn btn-neon btn-sm" href="/preorder" aria-current={cur('/preorder')}>
            Pre-order
          </Link>
          <button className="burger" type="button" aria-label="Menu" aria-expanded={open} aria-controls="menu" onClick={() => setOpen((o) => !o)}>
            <span />
            <span />
            <span />
          </button>
        </div>
      </div>
      <div className={`menu${open ? ' open' : ''}`} id="menu">
        <div className="wrap">
          <Link href="/" aria-current={cur('/')}>Home</Link>
          {NAV_LINKS.map((l) => (
            <Link key={l.href} href={l.href} aria-current={cur(l.href)}>
              {l.label}
            </Link>
          ))}
          <Link href="/preorder" aria-current={cur('/preorder')}>Pre-order</Link>
        </div>
      </div>
    </header>
  );
}
