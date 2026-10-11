import type { Metadata, Viewport } from 'next';
import { Archivo, Inter, JetBrains_Mono } from 'next/font/google';
import './globals.css';
import Nav from '@/components/Nav';
import Footer from '@/components/Footer';
import RevealObserver from '@/components/RevealObserver';
import { siteUrl } from '@/lib/site';

/* Archivo is a variable font with a width axis (62–125). 62% is the condensed poster type the brand uses. */
const archivo = Archivo({ subsets: ['latin'], axes: ['wdth'], display: 'swap', variable: '--font-archivo' });
const inter = Inter({ subsets: ['latin'], weight: ['400', '500', '600'], display: 'swap', variable: '--font-inter' });
const mono = JetBrains_Mono({ subsets: ['latin'], weight: ['400', '500', '600'], display: 'swap', variable: '--font-mono' });

export const metadata: Metadata = {
  metadataBase: new URL(siteUrl()),
  title: { default: 'Degen Walkie — Talk your book. Then trade it.', template: '%s — Degen Walkie' },
  description:
    'Degen Walkie, Genesis Edition. A perp trading terminal with a live, translated room for every ticker. Build orders by touch, send them with a physical trade key. Hyperliquid, Pacifica and Lighter. We never hold your money. $99, 1,000 units, pre-order refundable.',
  openGraph: { type: 'website', siteName: 'Degen Walkie' },
  twitter: { card: 'summary_large_image' },
};

export const viewport: Viewport = { themeColor: '#07080a', colorScheme: 'dark' };

export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="en" data-theme="dark" className={`${archivo.variable} ${inter.variable} ${mono.variable}`} suppressHydrationWarning>
      <head>
        {/* .js gates the scroll-reveal styles so content stays visible without JavaScript */}
        <script dangerouslySetInnerHTML={{ __html: "document.documentElement.classList.add('js')" }} />
      </head>
      <body>
        <Nav />
        <main id="top">{children}</main>
        <Footer />
        <RevealObserver />
      </body>
    </html>
  );
}
