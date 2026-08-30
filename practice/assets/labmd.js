/* labmd.js — md 파일을 실습 문서로 렌더링하는 뷰어 엔진
 * 강사 편집 규칙(md 안에서):
 *  - 첫 줄 H1: "# [실습ID] 제목"  → 상단 헤더에 배지+제목
 *  - "## Step 1. 제목 | 10분"     → 번호·체크박스·시간이 붙은 스텝 카드로 자동 변환
 *  - 문단 시작 이모지로 색 박스 지정: ✅(초록 결과) ⚠️(노랑 주의) 💡(파랑 팁) 🖱(회색 GUI 경로) ❓(왜? 회색)
 *  - "- [ ] 항목"                → Pass 체크리스트(초록 테두리 박스)
 *  - 코드 블록(```)             → 자동 복사 버튼
 */
(async function(){
  const qs=new URLSearchParams(location.search);
  const file=qs.get('file');
  const wrap=document.getElementById('wrap');
  async function loadText(){
    if(!file) throw new Error('no-file');
    const r=await fetch(file,{cache:'no-store'});
    if(!r.ok) throw new Error('fetch-fail '+r.status);
    return await r.text();
  }
  function render(mdText){
    marked.use({gfm:true,breaks:false});
    wrap.innerHTML=marked.parse(mdText);
    // 1) H1 → 헤더 배지/제목
    const h1=wrap.querySelector('h1');
    if(h1){
      const m=h1.textContent.match(/^\s*\[([^\]]+)\]\s*(.+)$/);
      document.getElementById('hid').textContent=m?('실습 '+m[1]):'실습';
      document.getElementById('htitle').textContent=m?m[2]:h1.textContent;
      document.title='🛠 '+(m?m[1]+' — '+m[2]:h1.textContent);
    }
    // 2) "## Step N. 제목 | 시간" → 스텝 카드
    wrap.querySelectorAll('h2').forEach(h2=>{
      const m=h2.textContent.match(/^Step\s*(\d+)\s*[.．]?\s*(.*?)(?:\s*\|\s*(.+))?$/i);
      if(!m) return;
      const card=document.createElement('div');card.className='step';
      const head=document.createElement('div');head.className='step-h';
      head.innerHTML=`<span class="n">${m[1]}</span><h3>${m[2]}</h3>`+
        (m[3]?`<span class="t">${m[3]}</span>`:'')+`<input type="checkbox" class="st">`;
      const body=document.createElement('div');body.className='step-b';
      let sib=h2.nextSibling;
      while(sib && !(sib.nodeType===1 && /^H2$/.test(sib.tagName))){const nx=sib.nextSibling;body.appendChild(sib);sib=nx;}
      card.appendChild(head);card.appendChild(body);
      h2.replaceWith(card);
    });
    // 3) 이모지 지시 문단 → 클래스
    const map=[['✅','expect'],['⚠️','warnbox'],['⚠','warnbox'],['💡','tip'],['🖱','gui'],['❓','why']];
    wrap.querySelectorAll('p').forEach(p=>{
      const t=p.textContent.trimStart();
      for(const [e,c] of map){ if(t.startsWith(e)){p.classList.add(c);break;} }
    });
    // 4) 작업 목록 체크박스 활성화
    wrap.querySelectorAll('.task-list-item input').forEach(i=>i.removeAttribute('disabled'));
    // 5) 코드 복사 버튼
    wrap.querySelectorAll('pre').forEach(pre=>{
      const b=document.createElement('button');b.className='copy';b.textContent='복사';
      b.onclick=()=>{navigator.clipboard.writeText(pre.querySelector('code').textContent.trim())
        .then(()=>{b.textContent='복사됨!';setTimeout(()=>b.textContent='복사',1200)});};
      pre.appendChild(b);
    });
    // 6) 진행률
    const boxes=[...wrap.querySelectorAll('.st')],pd=document.getElementById('pd'),pt=document.getElementById('pt');
    pt.textContent=boxes.length;
    boxes.forEach(b=>b.addEventListener('change',()=>{
      b.closest('.step').classList.toggle('done',b.checked);
      pd.textContent=boxes.filter(x=>x.checked).length;
    }));
    document.getElementById('progWrap').style.display=boxes.length?'':'none';
  }
  try{ render(await loadText()); }
  catch(e){
    wrap.innerHTML=`<div id="err"><h2>📄 md 파일을 불러올 수 없습니다</h2>
     <p style="color:#6b7280">웹 서버(GitHub Pages)에서는 <code>lab.html?file=lab_d0_1.md</code> 형태로 자동 로드됩니다.<br>
     로컬(file://)에서 미리보려면 아래 버튼으로 md 파일을 직접 선택하세요.</p>
     <input type="file" id="pick" accept=".md,.markdown,.txt"></div>`;
    document.getElementById('pick').onchange=ev=>{
      const f=ev.target.files[0]; if(!f)return;
      const r=new FileReader(); r.onload=()=>render(r.result); r.readAsText(f,'utf-8');
    };
  }
})();
