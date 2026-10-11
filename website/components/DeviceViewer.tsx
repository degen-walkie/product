'use client';

import { useEffect, useRef, useState } from 'react';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { buildDevice, createRenderer, createScene, FEATURES, fitDistance, hasWebGL, HOT, legendAt, loadGeometries, NEON, VIEWS, viewDir, type DeviceRig } from '@/lib/three/device';

export type ViewName = 'front' | 'back' | 'right' | 'top' | 'bottom';
export type LightMode = 'cycle' | 'green' | 'purple';

export interface DeviceViewerProps {
  view?: ViewName;
  autoRotate?: boolean;
  clip?: boolean;
  light?: LightMode;
  controls?: boolean; // preset buttons + clip/orbit toggles
  features?: boolean; // hardware-page callouts
  wide?: boolean;
  badge?: string;
  badgeSoon?: boolean;
  fit?: number; // >1 pulls the camera back (zoom out), e.g. 1.1 = 10% smaller
}

interface Api {
  goView(name: string): void;
  goFeature(id: string): void;
  setClip(on: boolean): void;
  setOrbit(on: boolean): void;
}

const RESUME_MS = 3000;

export default function DeviceViewer({ view = 'front', autoRotate = false, clip = false, light = 'green', controls = true, features = false, wide = false, badge = 'Live CAD render', badgeSoon = false, fit = 1 }: DeviceViewerProps) {
  const hostRef = useRef<HTMLDivElement>(null);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const labelRef = useRef<HTMLDivElement>(null);
  const apiRef = useRef<Api | null>(null);
  const [status, setStatus] = useState('Loading model…');
  const [ready, setReady] = useState(false);
  const [activeView, setActiveView] = useState<string | null>(view);
  const [clipOn, setClipOn] = useState(clip);
  const [orbitOn, setOrbitOn] = useState(autoRotate);
  const [feature, setFeature] = useState<string | null>(null);

  useEffect(() => {
    const host = hostRef.current, canvas = canvasRef.current, labelEl = labelRef.current;
    if (!host || !canvas) return;
    if (!hasWebGL()) { setStatus('Static preview'); return; }

    const reduceMotion = window.matchMedia?.('(prefers-reduced-motion: reduce)').matches ?? false;
    let disposed = false;
    const renderer = createRenderer(canvas);
    const scene = createScene(renderer);
    const camera = new THREE.PerspectiveCamera(24, 1, 10, 3000);
    const target = new THREE.Vector3(0, 0, 0);

    const orbit = new OrbitControls(camera, canvas);
    orbit.enableDamping = true;
    orbit.dampingFactor = 0.08;
    orbit.enablePan = false;
    orbit.rotateSpeed = 0.7;
    orbit.autoRotateSpeed = 1.3;
    orbit.autoRotate = false;
    orbit.target.copy(target);

    let baseDist = 400;
    let rig: DeviceRig | null = null;
    let orbitWanted = autoRotate && !reduceMotion; // user's intent (Orbit toggle)
    let resumeTimer: ReturnType<typeof setTimeout> | null = null;
    let activeFeature: string | null = null;

    // camera fly animation (presets / callouts)
    let fly: { fromPos: THREE.Vector3; toPos: THREE.Vector3; fromTgt: THREE.Vector3; toTgt: THREE.Vector3; t0: number; dur: number } | null = null;
    const now = () => performance.now() / 1000;
    const ease = (x: number) => 1 - Math.pow(1 - x, 3);

    function placeCamera(dir: THREE.Vector3, zoom: number, tgt: THREE.Vector3, animate: boolean) {
      const toPos = tgt.clone().add(dir.clone().multiplyScalar(baseDist / zoom));
      if (!animate || reduceMotion) {
        camera.position.copy(toPos); orbit.target.copy(tgt); orbit.update();
        return;
      }
      fly = { fromPos: camera.position.clone(), toPos, fromTgt: orbit.target.clone(), toTgt: tgt.clone(), t0: now(), dur: 0.9 };
    }

    function resize() {
      const r = host!.getBoundingClientRect();
      const w = Math.max(1, Math.round(r.width)), h = Math.max(1, Math.round(r.height));
      renderer.setSize(w, h, false);
      camera.aspect = w / h;
      const prevBase = baseDist;
      baseDist = fitDistance(camera) * fit;
      camera.updateProjectionMatrix();
      // keep the relative zoom across resizes
      const d = camera.position.distanceTo(orbit.target);
      const dir = camera.position.clone().sub(orbit.target).normalize();
      camera.position.copy(orbit.target).add(dir.multiplyScalar((d / prevBase) * baseDist));
      orbit.minDistance = baseDist * 0.45;
      orbit.maxDistance = baseDist * 2.2;
      orbit.update();
    }

    const startView = VIEWS[view] || VIEWS.front;
    camera.position.copy(target).add(viewDir(view).multiplyScalar(baseDist / startView.zoom));
    camera.lookAt(target);
    resize();
    const ro = 'ResizeObserver' in window ? new ResizeObserver(resize) : null;
    if (ro) ro.observe(host); else window.addEventListener('resize', resize);

    // auto-rotate pauses while the user interacts, resumes 3 s after they let go
    const onStart = () => { fly = null; orbit.autoRotate = false; if (resumeTimer) { clearTimeout(resumeTimer); resumeTimer = null; } clearFeature(); setActiveView(null); };
    const onEnd = () => { if (orbitWanted) { if (resumeTimer) clearTimeout(resumeTimer); resumeTimer = setTimeout(() => { if (!disposed && orbitWanted) orbit.autoRotate = true; }, RESUME_MS); } };
    orbit.addEventListener('start', onStart);
    orbit.addEventListener('end', onEnd);

    function clearFeature() {
      activeFeature = null;
      if (rig) rig.marker.visible = false;
      labelEl?.classList.remove('show');
      setFeature(null);
    }

    const api: Api = {
      goView(name) {
        const v = VIEWS[name]; if (!v) return;
        clearFeature();
        orbit.autoRotate = false; orbitWanted = false; setOrbitOn(false);
        placeCamera(viewDir(name), v.zoom, target, true);
        setActiveView(name);
        setStatus('Fixed view · ' + name);
      },
      goFeature(id) {
        const f = FEATURES.find((x) => x.id === id); if (!f || !rig) return;
        orbit.autoRotate = false; orbitWanted = false; setOrbitOn(false);
        const v = VIEWS[f.view];
        // feature position is in device coordinates; the device group sits at z = 5 in the scene
        const tgt = new THREE.Vector3(f.pos[0], f.pos[1], f.pos[2]).add(rig.device.position).multiplyScalar(0.85);
        placeCamera(viewDir(f.view), Math.max(1.6, v.zoom * 1.25), tgt, true);
        rig.marker.position.set(f.pos[0], f.pos[1], f.pos[2]);
        rig.marker.visible = true;
        activeFeature = id;
        setFeature(id);
        setActiveView(f.view);
        if (labelEl) { labelEl.textContent = f.label; labelEl.classList.add('show'); }
        setStatus(f.label);
      },
      setClip(on) {
        if (!rig) return;
        rig.clipGroup.visible = on;
        rig.pogoGroup.visible = !on;
        setClipOn(on);
      },
      setOrbit(on) {
        orbitWanted = on;
        orbit.autoRotate = on;
        setOrbitOn(on);
        if (on) { clearFeature(); setActiveView(null); setStatus('Auto-orbit · drag to rotate'); }
      },
    };
    apiRef.current = api;

    let visible = true;
    const io = 'IntersectionObserver' in window ? new IntersectionObserver((entries) => { visible = entries[0].isIntersecting; }, { threshold: 0.02 }) : null;
    io?.observe(host);

    const clock = new THREE.Clock();
    let t = 0, screenAt = 0, raf = 0;
    const tmpV = new THREE.Vector3();

    function frame() {
      raf = requestAnimationFrame(frame);
      if (disposed || !visible || document.hidden) return;
      const dt = Math.min(clock.getDelta(), 0.1);
      t += dt;

      if (fly) {
        const k = Math.min(1, (now() - fly.t0) / fly.dur);
        const e = ease(k);
        camera.position.lerpVectors(fly.fromPos, fly.toPos, e);
        orbit.target.lerpVectors(fly.fromTgt, fly.toTgt, e);
        if (k >= 1) fly = null;
      }
      orbit.update();

      if (rig) {
        if (light === 'cycle') {
          const L = legendAt(t);
          rig.setLight(L.color, L.k);
        } else {
          rig.setLight(light === 'purple' ? HOT : NEON, 0.75 + (reduceMotion ? 0 : 0.15 * Math.sin(t * 2)));
        }
        if (!reduceMotion && t - screenAt > 0.12) { rig.redrawScreen(t); screenAt = t; }
        if (activeFeature && labelEl) {
          rig.marker.getWorldPosition(tmpV).project(camera);
          const r = host!.getBoundingClientRect();
          labelEl.style.left = ((tmpV.x + 1) / 2) * r.width + 'px';
          labelEl.style.top = ((1 - tmpV.y) / 2) * r.height + 'px';
        }
      }
      renderer.render(scene, camera);
    }

    setStatus('Loading enclosure CAD…');
    const timeout = new Promise<never>((_, rej) => setTimeout(() => rej(new Error('timeout')), 30000));
    Promise.race([loadGeometries((n, total) => setStatus(`Loading enclosure CAD ${n}/${total}`)), timeout])
      .then((cache) => {
        if (disposed) return;
        rig = buildDevice(cache, renderer);
        rig.device.position.set(0, 0, 5); // centre the 10 mm thickness on the orbit target
        rig.clipGroup.visible = clip;
        rig.pogoGroup.visible = !clip;
        scene.add(rig.device);
        if (document.fonts?.ready) document.fonts.ready.then(() => { if (!disposed && rig) rig.redrawScreen(t); }).catch(() => {});
        renderer.render(scene, camera);
        setReady(true);
        orbit.autoRotate = orbitWanted;
        setStatus(orbitWanted ? 'Auto-orbit · drag to rotate' : 'Drag to rotate');
        frame();
      })
      .catch((e) => {
        console.warn('[viewer] static preview:', e);
        setStatus('Static preview');
      });

    return () => {
      disposed = true;
      cancelAnimationFrame(raf);
      if (resumeTimer) clearTimeout(resumeTimer);
      orbit.removeEventListener('start', onStart);
      orbit.removeEventListener('end', onEnd);
      orbit.dispose();
      ro?.disconnect();
      io?.disconnect();
      window.removeEventListener('resize', resize);
      rig?.dispose();
      renderer.dispose();
      apiRef.current = null;
    };
    // The viewer is configured once; prop changes re-mount via key in the parent if needed.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const viewerEl = (
    <div className="viewer-block">
      <div ref={hostRef} className={`viewer${wide ? ' wide' : ''}${ready ? ' is-3d' : ''}`} aria-label="Interactive 3D model of the Degen Walkie enclosure">
        <div className="device-mock" aria-hidden="true" />
        <canvas ref={canvasRef} aria-hidden="true" />
        <span className={`viewer-badge${badgeSoon ? ' soon' : ''}`}>{badge}</span>
        <span className="viewer-status">{status}</span>
        <div ref={labelRef} className="viewer-label" aria-hidden="true" />
      </div>
      {controls ? (
        <div className="viewer-controls" role="group" aria-label="Fixed views">
          {(['front', 'back', 'right', 'top'] as ViewName[]).map((v) => (
            <button key={v} className={`vbtn${activeView === v ? ' on' : ''}`} type="button" onClick={() => apiRef.current?.goView(v)}>
              {v[0].toUpperCase() + v.slice(1)}
            </button>
          ))}
          <span className="sep" aria-hidden="true" />
          <button
            className={`vbtn hot${clipOn ? ' on' : ''}`}
            type="button"
            aria-pressed={clipOn}
            onClick={() => {
              const on = !clipOn;
              apiRef.current?.setClip(on);
              if (on && !feature) apiRef.current?.goView('back');
            }}
          >
            With 4G clip
          </button>
          <span className="grow" />
          <button className={`vbtn${orbitOn ? ' on' : ''}`} type="button" aria-pressed={orbitOn} onClick={() => apiRef.current?.setOrbit(!orbitOn)}>
            Orbit
          </button>
        </div>
      ) : null}
    </div>
  );

  if (!features) return viewerEl;

  return (
    <div className="hw-layout">
      <div className="rv in">{viewerEl}</div>
      <ol className="callouts rv in" aria-label="Feature callouts">
        {FEATURES.map((f) => (
          <li key={f.id}>
            <button type="button" className={feature === f.id ? 'on' : ''} onClick={() => apiRef.current?.goFeature(f.id)}>
              <span>
                <b>{f.title}</b>
                <small>{f.text}</small>
              </span>
              <span className="where">{f.where}</span>
            </button>
          </li>
        ))}
      </ol>
    </div>
  );
}
