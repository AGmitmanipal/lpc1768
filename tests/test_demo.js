// Run with Node. Checks browser logic with a minimal DOM adapter; does not render a browser.
const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const html=fs.readFileSync(path.join(__dirname,'../demo/index.html'),'utf8');
const script=html.match(/<script>([\s\S]*?)<\/script>/)[1];
const vals={shape:'sine',inputHz:'100',amplitude:'2',rate:'5000',mode:'0',edge:'rise',threshold:'1.65',window:'200'};
const ctx=new Proxy({}, {get:()=>()=>{}}),elems={};
const el=id=>elems[id]||(elems[id]={value:vals[id]||'',textContent:'',addEventListener(){},getBoundingClientRect(){return {width:736,height:360}},getContext(){return ctx}});
const sandbox={document:{getElementById:el},window:{devicePixelRatio:1,addEventListener(){}},setInterval(){},setTimeout(){},console,Blob,URL};
vm.createContext(sandbox);vm.runInContext(script,sandbox);
const t=sandbox.window.DSOTest,c=t.config();
const r=t.acquire(c);assert(r&&r.a.length===1000&&!r.forced);
const m=t.measurements(r.a,c);assert(Math.abs(m.f-100)<0.1);assert(m.vpp>1.99&&m.vpp<2.01);
assert(t.acquire({...c,mode:1,shape:'dc'})===null);
assert(t.acquire({...c,mode:0,shape:'dc'}).forced);
assert(t.acquire({...c,mode:2,shape:'dc'})===null);
assert(t.acquire({...c,fall:true}));assert(t.measurements(t.acquire({...c,amp:4}).a,c).clip);
const fft=t.computeFFT(r.a,c.fs);
assert(fft.peak>=4&&fft.peak<=6);
assert(Math.abs(fft.peakFreq-100)<25);
console.log('PASS: browser script startup, sine measurement, modes, falling edge, clipping, FFT peak');

