function initAnim(cfg){
  const svg=document.getElementById('stage');
  const steps=cfg.steps, narr=document.getElementById('narr'),
        term=document.getElementById('term'), lbl=document.getElementById('steplbl'),
        dots=document.querySelector('.dots');
  let i=-1, gen=0, timer=null;
  steps.forEach((_,k)=>{const d=document.createElement('span');d.onclick=()=>{stopAuto();go(k)};dots.appendChild(d)});
  function clearFx(){gen++;
    svg.querySelectorAll('.on').forEach(e=>e.classList.remove('on'));
    svg.querySelectorAll('.flow').forEach(e=>e.classList.remove('flow','red','grn'));
    svg.querySelectorAll('.pkt').forEach(e=>e.remove());
    svg.querySelectorAll('.hidden-ctl').forEach(e=>e.classList.add('hid'));
    svg.classList.remove('dead');
  }
  function pkt(p,g){const path=svg.getElementById(p.path); if(!path)return;
    const c=document.createElementNS('http://www.w3.org/2000/svg','circle');
    c.setAttribute('r',p.r||7); c.setAttribute('class','pkt '+(p.cls||''));
    svg.appendChild(c);
    const len=path.getTotalLength(), dur=p.dur||1500, delay=p.delay||0, t0=performance.now()+delay;
    function f(t){ if(g!==gen){c.remove();return}
      const u=Math.max(0,Math.min(1,(t-t0)/dur));
      const pos=path.getPointAtLength((p.rev?1-u:u)*len);
      c.setAttribute('cx',pos.x); c.setAttribute('cy',pos.y);
      if(u<1)requestAnimationFrame(f);
      else if(p.loop){t0r=performance.now()+ (p.gap||250); (function w(t2){ if(g!==gen){c.remove();return} if(t2>=t0r){pkt(p,g);c.remove()} else requestAnimationFrame(w)})(performance.now());}
      else c.remove(); }
    let t0r; requestAnimationFrame(f);
  }
  function go(k){ if(k<0||k>=steps.length)return; i=k; clearFx(); const st=steps[i];
    (st.on||[]).forEach(id=>{const e=svg.getElementById(id); if(e)e.classList.add('on')});
    (st.show||[]).forEach(id=>{const e=svg.getElementById(id); if(e)e.classList.remove('hid')});
    (st.arrows||[]).forEach(a=>{const id=a.split(':')[0],cls=a.split(':')[1];const e=svg.getElementById(id);
       if(e){e.classList.add('flow'); if(cls)e.classList.add(cls)}});
    (st.pkts||[]).forEach(p=>pkt(p,gen));
    if(st.dead)svg.classList.add('dead');
    narr.innerHTML='<b>Step '+(i+1)+'/'+steps.length+' · '+(st.t||'')+'</b><br>'+(st.n||'');
    if(st.c){term.style.display='block';term.innerHTML='<pre>'+st.c+'</pre>'}else{term.style.display='none'}
    lbl.textContent=(i+1)+' / '+steps.length;
    dots.querySelectorAll('span').forEach((d,j)=>d.classList.toggle('cur',j===i));
  }
  function stopAuto(){if(timer){clearInterval(timer);timer=null;ab.textContent='▶▶ 자동재생';ab.classList.remove('play')}}
  const ab=document.getElementById('auto');
  document.getElementById('prev').onclick=()=>{stopAuto();go(i-1)};
  document.getElementById('next').onclick=()=>{stopAuto();go(i+1)};
  ab.onclick=()=>{ if(timer){stopAuto();return}
    ab.textContent='⏸ 정지';ab.classList.add('play');
    if(i>=steps.length-1)go(0);
    timer=setInterval(()=>{ if(i>=steps.length-1)stopAuto(); else go(i+1)},cfg.autoMs||3200)};
  document.addEventListener('keydown',e=>{ if(e.key==='ArrowRight'){stopAuto();go(i+1)}
    if(e.key==='ArrowLeft'){stopAuto();go(i-1)}});
  go(0);
}
