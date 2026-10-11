import { ImageResponse } from 'next/og';

export const runtime = 'edge';
export const alt = 'Degen Walkie — Talk your book. Then trade it.';
export const size = { width: 1200, height: 630 };
export const contentType = 'image/png';

/* Wordmark on black. System sans-serif (no font fetch at the edge). */
export default function OpengraphImage() {
  return new ImageResponse(
    (
      <div style={{ width: '100%', height: '100%', display: 'flex', flexDirection: 'column', justifyContent: 'space-between', background: '#07080a', color: '#f4f5f7', padding: 72, fontFamily: 'Arial, Helvetica, sans-serif' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: 18, fontSize: 30, letterSpacing: 2, textTransform: 'uppercase', color: '#b6bcc6' }}>
          <div style={{ width: 22, height: 22, borderRadius: 999, background: '#14f195', boxShadow: '0 0 30px #14f195' }} />
          Genesis Edition · 1,000 units · $99
        </div>
        <div style={{ display: 'flex', flexDirection: 'column' }}>
          <div style={{ fontSize: 150, fontWeight: 900, lineHeight: 0.92, letterSpacing: -4, textTransform: 'uppercase' }}>Degen Walkie</div>
          <div style={{ display: 'flex', fontSize: 56, fontWeight: 800, lineHeight: 1.05, textTransform: 'uppercase', marginTop: 26 }}>
            <span>Talk your book.&nbsp;</span>
            <span style={{ color: '#14f195' }}>Then trade it.</span>
          </div>
        </div>
        <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: 26, color: '#7a828e', letterSpacing: 1 }}>
          <span>Perp terminal · a live room per ticker · 12 languages · we never hold your money</span>
          <span style={{ color: '#9945ff' }}>AI proposes · you press</span>
        </div>
      </div>
    ),
    { ...size },
  );
}
