'use client';

import { useEffect, useRef, useState } from 'react';
import * as THREE from 'three';
import { buildDevice, createRenderer, createScene, fitDistance, hasWebGL, loadGeometries, NEON, type DeviceRig } from '@/lib/three/device';

/* Scroll-driven assembly: the 4G clip starts ~45 mm behind the device and glides onto the six gold
   contacts as the section scrolls; after the click it rotates together with the device.
   Progress is read from scrollY (passive listener) — no scroll-jacking. */

const CARDS = [
  { idx: 'Step 1 · approach', k: '45 mm', title: 'Floating behind the back cover', text: 'The clip is its own device: 4G modem, battery, nano-SIM. It lives on the back of the Walkie, never inside it.' },
  { idx: 'Step 2 · align', k: '6 pins', title: 'Six spring pins meet six gold pads', text: 'The pogo row at y −20 lines up with the contacts on the back cover. Magnets pull the last millimetres.' },
  { idx: 'Step 3 · click', k: '0 mm', title: 'Snapped. Signal.', text: 'The device talks to the clip over the contacts and simply has cellular. Rooms, prices and trades without Wi-Fi.' },
  { idx: 'Together', k: '+6 mm', title: 'One object again', text: '60 × 78 × 6 mm, centred low so the camera stays clear. Sold separately; every Genesis unit ships with the pads and the magnet steel.' },
];

const SNAP_AT = 0.68; // progress at which the clip finishes travelling

function easeOutCubic(x: number) { return 1 - Math.pow(1 - x, 3); }
function easeOutBack(x: number) { const c1 = 1.70158, c3 = c1 + 1; return 1 + c3 * Math.pow(x - 1, 3) + c1 * Math.pow(x - 1, 2); }

export default function ClipScroll() {
  const sectionRef = useRef<HTMLDivElement>(null);
  const hostRef = useRef<HTMLDivElement>(null);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const barRef = useRef<HTMLElement>(null);
  const [status, setStatus] = useState('Loading model…');
  const [ready, setReady] = useState(false);
  const [card, setCard] = useState(0);

  useEffect(() => {
    const section = sectionRef.current, host = hostRef.current, canvas = canvasRef.current;
    if (!section || !host || !canvas) return;
    if (!hasWebGL()) { setStatus('Static preview'); return; }
    const reduceMotion = window.matchMedia?.('(prefers-reduced-motion: reduce)').matches ?? false;

    let disposed = false;
    const renderer = createRenderer(canvas);
    const scene = createScene(renderer);
    const camera = new THREE.PerspectiveCamera(26, 1, 10, 3000);
    const pivot = new THREE.Group();
    scene.add(pivot);
    let rig: DeviceRig | null = null;
    let baseDist = 420;

    function resize() {
      const r = host!.getBoundingClientRect();
      const w = Math.max(1, Math.round(r.width)), h = Math.max(1, Math.round(r.height));
      renderer.setSize(w, h, false);
      camera.aspect = w / h;
      baseDist = fitDistance(camera, 120 * 1.05, 150 * 1.02);
      camera.updateProjectionMatrix();
    }
    resize();
    const ro = 'ResizeObserver' in window ? new ResizeObserver(resize) : null;
    if (ro) ro.observe(host); else window.addEventListener('resize', resize);

    // scroll progress through the section
    let progress = 0;
    let lastCard = -1;
    function readScroll() {
      const r = section!.getBoundingClientRect();
      const vh = window.innerHeight || 1;
      const total = Math.max(1, r.height - vh);
      progress = THREE.MathUtils.clamp(-r.top / total, 0, 1);
      if (barRef.current) barRef.current.style.transform = `scaleX(${progress})`;
      const c = progress < 0.3 ? 0 : progress < SNAP_AT - 0.08 ? 1 : progress < 0.85 ? 2 : 3;
      if (c !== lastCard) { lastCard = c; setCard(c); }
    }
    readScroll();
    window.addEventListener('scroll', readScroll, { passive: true });

    let visible = true;
    const io = 'IntersectionObserver' in window ? new IntersectionObserver((entries) => { visible = entries[0].isIntersecting; }, { threshold: 0.01 }) : null;
    io?.observe(host);

    const clock = new THREE.Clock();
    let t = 0, raf = 0, spin = 0;
    const camDir = new THREE.Vector3(Math.sin(Math.PI - 0.55) * Math.cos(0.18), Math.sin(0.18), Math.cos(Math.PI - 0.55) * Math.cos(0.18)).normalize(); // behind, slightly right and above

    function frame() {
      raf = requestAnimationFrame(frame);
      if (disposed || !visible || document.hidden || !rig) return;
      const dt = Math.min(clock.getDelta(), 0.1);
      t += dt;

      // clip travel: separated → snapped (ease-out with a small overshoot "click" at the end)
      const u = THREE.MathUtils.clamp(progress / SNAP_AT, 0, 1);
      const travel = u < 0.85 ? easeOutCubic(u / 0.85) * 0.94 : 0.94 + 0.06 * easeOutBack((u - 0.85) / 0.15);
      const k = 1 - travel; // 1 = far, 0 = seated
      rig.clipGroup.position.set(14 * k, -8 * k, -45 * k);
      rig.clipGroup.rotation.set(0.18 * k, 0.32 * k, 0);
      rig.pogoGroup.visible = k > 0.08; // pads show until the pins cover them

      // after the click, the whole assembly rotates slowly together
      const after = THREE.MathUtils.clamp((progress - SNAP_AT) / (1 - SNAP_AT), 0, 1);
      if (!reduceMotion && after > 0) spin += dt * 0.35 * after;
      pivot.rotation.set(0.06 * after, spin + after * 0.9, 0);
      // small drift before the snap so the scene is alive
      const idle = reduceMotion ? 0 : Math.sin(t * 0.6) * 0.04 * (1 - after);
      pivot.rotation.y += idle;

      // camera: pull back slightly while separated so both parts fit
      const dist = baseDist * (1 + 0.22 * k);
      camera.position.copy(camDir).multiplyScalar(dist).add(new THREE.Vector3(0, -4 * k, 0));
      camera.lookAt(0, -2 * k, -12 * k);

      rig.setLight(NEON, 0.8 + 0.15 * Math.sin(t * 2));
      renderer.render(scene, camera);
    }

    setStatus('Loading enclosure CAD…');
    loadGeometries((n, total) => setStatus(`Loading enclosure CAD ${n}/${total}`))
      .then((cache) => {
        if (disposed) return;
        rig = buildDevice(cache, renderer);
        rig.device.position.set(0, 0, 5);
        rig.clipGroup.visible = true;
        pivot.add(rig.device);
        if (document.fonts?.ready) document.fonts.ready.then(() => { if (!disposed && rig) rig.redrawScreen(0); }).catch(() => {});
        setReady(true);
        setStatus('Scroll to snap the clip on');
        frame();
      })
      .catch((e) => { console.warn('[clip]', e); setStatus('Static preview'); });

    return () => {
      disposed = true;
      cancelAnimationFrame(raf);
      window.removeEventListener('scroll', readScroll);
      window.removeEventListener('resize', resize);
      ro?.disconnect();
      io?.disconnect();
      rig?.dispose();
      renderer.dispose();
    };
  }, []);

  return (
    <div ref={sectionRef} className="cs-grid">
      <div className="cs-sticky">
        <div ref={hostRef} className={`viewer${ready ? ' is-3d' : ''}`} aria-label="Scroll-driven animation of the 4G clip snapping onto the device">
          <div className="device-mock" aria-hidden="true" />
          <canvas ref={canvasRef} aria-hidden="true" />
          <div className="cs-progress" aria-hidden="true"><i ref={barRef} /></div>
          <span className="viewer-badge soon">Coming soon</span>
          <span className="viewer-status">{status}</span>
        </div>
      </div>
      <div className="cs-cards">
        {CARDS.map((c, i) => (
          <div key={c.idx} className={`cs-card${card === i ? ' on' : ''}`}>
            <div className="card still">
              <div className="idx">{c.idx}</div>
              <div className="k">{c.k}</div>
              <h3>{c.title}</h3>
              <p>{c.text}</p>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
