import * as THREE from 'https://unpkg.com/three@0.159.0/build/three.module.js';

/* ===============================
   VOXEL MODEL
   =============================== */

const SIZE = 15;
const PERIOD = 6.0;

const norm = k => (k / (SIZE - 1)) - 0.5;

function hsv2rgb(h, s, v) {
  const i = Math.floor(h * 6);
  const f = h * 6 - i;
  const p = v * (1 - s);
  const q = v * (1 - f * s);
  const t = v * (1 - (1 - f) * s);
  const m = i % 6;
  return [
    [v, q, p, p, t, v][m],
    [t, v, v, q, p, p][m],
    [p, p, t, v, v, q][m]
  ];
}

function voxelColor(x, y, z, time) {
  const px = norm(x), py = norm(y), pz = norm(z);
  const dist = Math.hypot(px, py, pz);
  const radius = ((time / PERIOD) % 1) * 0.9;
  const band = Math.max(0, 1 - Math.abs(dist - radius) / 0.04);
  const hue = (dist * 3 + time * 0.1) % 1;
  const [r, g, b] = hsv2rgb(hue, 1, band);
  return `rgb(${r * 255 | 0},${g * 255 | 0},${b * 255 | 0})`;
}

/* ===============================
   2D SLICE RENDERING
   =============================== */

function renderSlice(table, mapFn, layer, time, hl) {
  let html = '';
  for (let row = 0; row < SIZE; row++) {
    html += '<tr>';
    for (let col = 0; col < SIZE; col++) {
      const [x, y, z] = mapFn(col, row, layer);
      let cls = '';
      if (hl.x === x) cls += ' slice-x';
      if (hl.y === y) cls += ' slice-y';
      if (hl.z === z) cls += ' slice-z';
      html += `<td class="${cls}" style="background:${voxelColor(x, y, z, time)}"></td>`;
    }
    html += '</tr>';
  }
  table.innerHTML = html;
}

const viewTop   = document.getElementById('viewTop');
const viewSideX = document.getElementById('viewSideX');
const viewSideY = document.getElementById('viewSideY');

const layerX = document.getElementById('layerX');
const layerY = document.getElementById('layerY');
const layerZ = document.getElementById('layerZ');

const topMap   = (x, y, z) => [x, y, z];
const sideXMap = (x, y, l) => [l, y, x];
const sideYMap = (x, y, l) => [x, l, y];

/* ===============================
   3D VIEW
   =============================== */

const canvas = document.getElementById('voxelCanvas');
const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
const scene = new THREE.Scene();
scene.background = new THREE.Color(0x000000);

const camera = new THREE.PerspectiveCamera(45, 1, 0.1, 100);
camera.position.set(1.5, 1.5, 2.5);

const positions = [];
for (let z = 0; z < SIZE; z++)
  for (let y = 0; y < SIZE; y++)
    for (let x = 0; x < SIZE; x++)
      positions.push(norm(x), norm(y), norm(z));

const geom = new THREE.BufferGeometry();
geom.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));

const mat = new THREE.PointsMaterial({ size: 0.06, color: 0xffffff });
const points = new THREE.Points(geom, mat);
scene.add(points);

function resize() {
  const r = canvas.getBoundingClientRect();
  renderer.setSize(r.width, r.height, false);
  camera.aspect = r.width / r.height;
  camera.updateProjectionMatrix();
}
window.addEventListener('resize', resize);
resize();

/* ===============================
   MAIN LOOP
   =============================== */

const t0 = performance.now();

function tick() {
  const t = (performance.now() - t0) / 1000;

  const lx = layerX.value | 0;
  const ly = layerY.value | 0;
  const lz = layerZ.value | 0;

  // Top view (XY @ Z) → show X and Y guides only
  renderSlice(
    viewTop,
    topMap,
    lz,
    t,
    { x: lx, y: ly }       // ✅ no Z highlight here
  );

  // Side X view (YZ @ X) → show Y and Z guides only
  renderSlice(
    viewSideX,
    sideXMap,
    lx,
    t,
    { y: ly, z: lz }       // ✅ no X highlight here
  );

  // Side Y view (XZ @ Y) → show X and Z guides only
  renderSlice(
    viewSideY,
    sideYMap,
    ly,
    t,
    { x: lx, z: lz }       // ✅ no Y highlight here
  );

  renderer.render(scene, camera);
  requestAnimationFrame(tick);
}

tick();