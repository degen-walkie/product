'use client';

import dynamic from 'next/dynamic';
import type { DeviceViewerProps } from './DeviceViewer';

/* three.js only runs in the browser: load the viewer client-side, show the CSS silhouette meanwhile. */
const DeviceViewer = dynamic(() => import('./DeviceViewer'), {
  ssr: false,
  loading: () => (
    <div className="viewer-block">
      <div className="viewer" aria-label="3D model loading">
        <div className="device-mock" aria-hidden="true" />
        <span className="viewer-status">Loading model…</span>
      </div>
    </div>
  ),
});

export default function Viewer(props: DeviceViewerProps) {
  return <DeviceViewer {...props} />;
}
