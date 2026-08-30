function initWS(cfg){
 const root=document.getElementById('ws'); let html='';
 cfg.chapters.forEach((ch,ci)=>{
  html+=`<section class="chap" id="chap${ci}"><h2>${ch.title}</h2><div class="chsub">${ch.sub||''}</div>`;
  ch.questions.forEach((q,qi)=>{
   const qid=`q${ci}_${qi}`;
   html+=`<div class="q" id="${qid}"><span class="mark"></span><div class="qt"><span class="pt">[${ci+1}-${qi+1}]</span> ${q.q}${q.anim?` <a class="alink" href="${q.anim}" target="_blank">🎬 애니메이션</a>`:''}</div>`;
   if(q.type==='mc'){ html+=q.opts.map((o,oi)=>`<label class="opt"><input type="radio" name="${qid}" value="${oi}">${o}</label>`).join(''); }
   else if(q.type==='blank'){ html+=`<input class="blank" id="${qid}_in" placeholder="답을 입력하세요" autocomplete="off">`; }
   else if(q.type==='order'){
    html+=`<div class="order" id="${qid}_ord">`+q.items.map((it,ii)=>`<button type="button" class="oitem" data-i="${ii}">${it}</button>`).join('')+`</div>
    <div class="osel">클릭한 순서대로 번호가 붙습니다 <button type="button" class="oreset" data-q="${qid}">↺ 다시 선택</button></div>`; }
   else if(q.type==='match'){
    html+=q.pairs.map((p,pi)=>`<div class="mrow">${p[0]} &nbsp;→&nbsp;<select id="${qid}_m${pi}"><option value="">-- 선택 --</option>${q.opts.map((o,oi)=>`<option value="${oi}">${o}</option>`).join('')}</select></div>`).join(''); }
   html+=`<div class="exp" id="${qid}_exp"></div></div>`;
  });
  html+=`</section>`;
 });
 html+=`<div class="gradebar"><input id="stuName" placeholder="이름"><button id="gradeBtn">✅ 채점하기</button><button id="retryBtn">↺ 다시 풀기</button><button onclick="window.print()">🖨 인쇄/제출</button><div id="result"></div></div>`;
 root.innerHTML=html;
 // ordering interaction
 const ordState={};
 root.querySelectorAll('.order').forEach(o=>{ordState[o.id]=[];
  o.querySelectorAll('.oitem').forEach(b=>b.onclick=()=>{ if(b.classList.contains('sel'))return;
   b.classList.add('sel'); ordState[o.id].push(+b.dataset.i);
   const n=document.createElement('span');n.className='num';n.textContent=ordState[o.id].length;b.appendChild(n);});});
 root.querySelectorAll('.oreset').forEach(r=>r.onclick=()=>{const oid=r.dataset.q+'_ord';ordState[oid]=[];
  document.getElementById(oid).querySelectorAll('.oitem').forEach(b=>{b.classList.remove('sel');const n=b.querySelector('.num');if(n)n.remove();});});
 function norm(s){return (s||'').toLowerCase().replace(/[\s_()\-\.]/g,'')}
 function grade(){
  let tot=0,got=0;const subs=[];
  cfg.chapters.forEach((ch,ci)=>{let cTot=0,cGot=0;
   ch.questions.forEach((q,qi)=>{const qid=`q${ci}_${qi}`,el=document.getElementById(qid);
    el.classList.remove('good','bad');let ok=false;
    if(q.type==='mc'){const s=root.querySelector(`input[name="${qid}"]:checked`);ok=s&&+s.value===q.ans;}
    else if(q.type==='blank'){const v=norm(document.getElementById(qid+'_in').value);ok=q.ans.some(a=>norm(a)===v)||q.ans.some(a=>v&&v.includes(norm(a))&&norm(a).length>=3);}
    else if(q.type==='order'){const sel=ordState[qid+'_ord']||[];ok=sel.length===q.ans.length&&sel.every((v,k)=>v===q.ans[k]);}
    else if(q.type==='match'){ok=q.pairs.every((p,pi)=>{const s=document.getElementById(`${qid}_m${pi}`);return s&&+s.value===p[1];});}
    el.classList.add(ok?'good':'bad');el.querySelector('.mark').textContent=ok?'✅':'❌';
    const ex=document.getElementById(qid+'_exp');ex.style.display='block';
    let ansTxt='';
    if(q.type==='mc')ansTxt=`<b>정답:</b> ${q.opts[q.ans]}`;
    if(q.type==='blank')ansTxt=`<b>정답:</b> <code>${q.ans[0]}</code>`;
    if(q.type==='order')ansTxt=`<b>정답 순서:</b> ${q.ans.map(i=>q.items[i]).join(' → ')}`;
    if(q.type==='match')ansTxt=`<b>정답:</b> ${q.pairs.map(p=>p[0].replace(/<[^>]+>/g,'')+'→'+q.opts[p[1]]).join(' · ')}`;
    ex.innerHTML=ansTxt+'<br>'+(q.exp||'');
    cTot++;tot++;if(ok){cGot++;got++}});
   subs.push(`${ch.short||ch.title.split(' ')[0]} ${cGot}/${cTot}`);});
  const name=document.getElementById('stuName').value||'(이름 미입력)';
  const pct=Math.round(got/tot*100);
  document.getElementById('result').innerHTML=`<span class="tot">${name}: ${got}/${tot} (${pct}점)</span><span class="sub">${subs.join(' · ')} — ${pct>=80?'🎉 통과! 다음 챕터로':'📖 ❌ 문항의 애니메이션을 다시 보고 재도전하세요'}</span>`;
  document.getElementById('result').scrollIntoView({behavior:'smooth',block:'center'});
 }
 document.getElementById('gradeBtn').onclick=grade;
 document.getElementById('retryBtn').onclick=()=>location.reload();
}
