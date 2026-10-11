'use client';

import dynamic from 'next/dynamic';

const ClipScroll = dynamic(() => import('./ClipScroll'), {
  ssr: false,
  loading: () => (
    <div className="cs-grid">
      <div className="cs-sticky">
        <div className="viewer" aria-label="3D model loading">
          <div className="device-mock" aria-hidden="true" />
          <span className="viewer-status">Loading model…</span>
        </div>
      </div>
      <div className="cs-cards" />
    </div>
  ),
});

export default function ClipScrollSection() {
  return <ClipScroll />;
}
