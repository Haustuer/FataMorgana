import * as THREE from 'https://unpkg.com/three@0.159.0/build/three.module.js';

const SIZE=15;const PERIOD=6.0;
const norm=k=>(k/(SIZE-1))-0.5;
const hsv2rgb=(h,s,v)=>{const i=Math.floor(h*6),f=h*6-i;const p=v*(1-s),q=v*(1-f*s),t=v*(1-(1-f)*s);const m=i%6;return[[v,q,p,p,t,v][m],[t,v,v,q,p,p][m],[p,p,t,v,v,q][m]]};
function voxelColor(x,y,z,t){const px=norm(x),py=norm(y),pz=norm(z);const d=Math.hypot(px,py,pz);const r=((t/PERIOD)%1)*0.9;const band=Math.max(0,1-Math.abs(d-r)/0.04);const hue=(d*3+t*0.1)%1;const [R,G,B]=hsv2rgb(hue,1,band);return `rgb(${R*255|0},${G*255|0},${B*255|0})`;}

function renderSlice(tbl,map,layer,t){let h='';for(let y=0;y<SIZE;y++){h+='<tr>';for(let x=0;x<SIZE;x++){const [vx,vy,vz]=map(x,y,layer);h+=`<td style="background:${voxelColor(vx,vy,vz,t)}"></td>`;}h+='</tr>';}tbl.innerHTML=h;}

const viewTop=document.getElementById('viewTop');
const viewSideX=document.getElementById('viewSideX');
const viewSideY=document.getElementById('viewSideY');
const layerZ=document.getElementById('layerZ');
const layerX=document.getElementById('layerX');
const layerY=document.getElementById('layerY');

const topMap=(x,y,z)=>[x,y,z];
const sideXMap=(x,y,l)=>[l,y,x];
const sideYMap=(x,y,l)=>[x,l,y];

const t0=performance.now();
function tick(){const t=(performance.now()-t0)/1000;renderSlice(viewTop,topMap,layerZ.value|0,t);renderSlice(viewSideX,sideXMap,layerX.value|0,t);renderSlice(viewSideY,sideYMap,layerY.value|0,t);requestAnimationFrame(tick);}tick();

// 3D view
const canvas=document.getElementById('voxelCanvas');
const renderer=new THREE.WebGLRenderer({canvas});
const scene=new THREE.Scene();scene.background=new THREE.Color(0x000000);
const camera=new THREE.PerspectiveCamera(45,1,0.1,100);camera.position.set(1.5,1.5,2.5);
const geom=new THREE.BufferGeometry();const pos=[];
for(let z=0;z<SIZE;z++)for(let y=0;y<SIZE;y++)for(let x=0;x<SIZE;x++){pos.push(norm(x),norm(y),norm(z));}
geom.setAttribute('position',new THREE.Float32BufferAttribute(pos,3));
const mat=new THREE.PointsMaterial({size:0.05,vertexColors:false,color:0xffffff});
const pts=new THREE.Points(geom,mat);scene.add(pts);
function resize(){const r=canvas.getBoundingClientRect();renderer.setSize(r.width,r.height,false);camera.aspect=r.width/r.height;camera.updateProjectionMatrix();}
window.addEventListener('resize',resize);resize();
(function loop(){renderer.render(scene,camera);requestAnimationFrame(loop);})();
