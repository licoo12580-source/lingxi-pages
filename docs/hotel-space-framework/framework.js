(() => {
  const app = document.getElementById('app');

  const paths = {
    trend: '<path d="M4 19V9m6 10V5m6 14v-7m5 7H2"/>',
    spaces: '<rect x="3" y="3" width="7" height="7" rx="1"/><rect x="14" y="3" width="7" height="7" rx="1"/><rect x="3" y="14" width="7" height="7" rx="1"/><rect x="14" y="14" width="7" height="7" rx="1"/>',
    work: '<path d="m12 2 9 5-9 5-9-5zM3 12l9 5 9-5M3 17l9 5 9-5"/>',
    device: '<rect x="7" y="7" width="10" height="10" rx="2"/><path d="M9 1v3m6-3v3M9 20v3m6-3v3M20 9h3m-3 6h3M1 9h3m-3 6h3"/>',
    control: '<path d="m13 2-9 12h8l-1 8 9-12h-8z"/>',
    proof: '<path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><path d="M14 2v6h6M8 13h8M8 17h6"/>',
    search: '<circle cx="11" cy="11" r="7"/><path d="m20 20-4-4"/>',
    bell: '<path d="M18 8a6 6 0 0 0-12 0c0 7-3 7-3 9h18c0-2-3-2-3-9M10 21h4"/>',
    chevron: '<path d="m9 18 6-6-6-6"/>',
    filter: '<path d="M4 21v-7m0-4V3m8 18v-9m0-4V3m8 18v-5m0-4V3M1 14h6M9 8h6m2 8h6"/>',
    close: '<path d="M18 6 6 18M6 6l12 12"/>',
    menu: '<path d="M4 6h16M4 12h16M4 18h16"/>',
    arrow: '<path d="M5 12h14m-5-5 5 5-5 5"/>'
  };
  const icon = (name) => `<svg class="icon" viewBox="0 0 24 24" aria-hidden="true">${paths[name]}</svg>`;

  const spaces = [
    { no: '1001', model: 'LD2450', observation: '1 人候选 · 12 分钟', env: '板内温湿度 · 未校准', ability: '空调能力样本', state: 'ok', updated: '09:44:18' },
    { no: '1002', model: 'MS24', observation: '活动候选 · 6 分钟', env: '板内温湿度 · 未校准', ability: '未匹配控制', state: 'ok', updated: '09:44:18' },
    { no: '1003', model: 'LD2453', observation: '0 人候选 · 28 分钟', env: '相对偏暗', ability: '电视能力样本', state: 'ok', updated: '09:44:18' },
    { no: '1005', model: '—', observation: '数据中断 · 18 分钟', env: '证据同时中断', ability: '控制已阻断', state: 'gap', updated: '09:24:03' },
    { no: '1006', model: 'MS24', observation: '活动候选 · 刚变化', env: '相对变亮', ability: '未匹配控制', state: 'ok', updated: '09:44:18' },
    { no: '1007', model: '—', observation: '设备身份待核对', env: '无法判断', ability: '不能控制', state: 'unknown', updated: '—' },
    { no: '1008', model: 'LD2450', observation: '2 人候选 · 4 分钟', env: '板内温湿度 · 未校准', ability: '空调能力样本', state: 'attention', updated: '09:44:18' },
    { no: '1009', model: 'MS24', observation: '无活动候选 · 35 分钟', env: '相对微亮', ability: '未匹配控制', state: 'ok', updated: '09:44:18' },
    { no: '1010', model: '—', observation: '未接入观察能力', env: '无数据', ability: '不能控制', state: 'empty', updated: '—' }
  ];

  const state = { view: 'overview', selectedSpace: spaces[0], mobileMenu: false };

  app.innerHTML = `
    <div class="shell">
      <aside class="sidebar">
        <a class="brand" href="#" data-view="overview"><span class="brand-mark"><i></i><i></i><i></i></span><span><b>灵犀</b><small>酒店空间总控</small></span></a>
        <nav class="main-nav" aria-label="主要功能">
          <button class="active" data-view="overview">${icon('trend')}<span>集团态势</span></button>
          <button data-view="spaces">${icon('spaces')}<span>酒店空间</span></button>
          <button data-view="work">${icon('work')}<span>工作中心</span><em>3</em></button>
          <button data-view="devices">${icon('device')}<span>节点与设备</span></button>
          <button data-view="control">${icon('control')}<span>能力与控制</span></button>
          <button data-view="evidence">${icon('proof')}<span>证据与运维</span></button>
        </nav>
        <div class="automation-off"><span>无人值守控制</span><b>关闭</b></div>
      </aside>

      <div class="page-column">
        <header class="topbar">
          <button class="mobile-menu" type="button" aria-label="打开导航">${icon('menu')}</button>
          <div class="scope">
            <button>灵犀示例集团 ${icon('chevron')}</button><i></i>
            <button>广州天河店 ${icon('chevron')}</button><i></i>
            <button>1 号楼 · 1F ${icon('chevron')}</button>
          </div>
          <div class="top-actions"><span class="sample">布局样本</span><button aria-label="搜索">${icon('search')}</button><button class="notification" aria-label="待处理 3 项">${icon('bell')}<b>3</b></button><small>更新于 09:44</small></div>
        </header>
        <main id="workspace" class="workspace"></main>
      </div>

      <nav class="bottom-nav" aria-label="手机导航">
        <button class="active" data-view="overview">${icon('trend')}<span>态势</span></button>
        <button data-view="spaces">${icon('spaces')}<span>空间</span></button>
        <button data-view="devices">${icon('device')}<span>设备</span></button>
        <button data-view="control">${icon('control')}<span>控制</span></button>
        <button data-view="evidence">${icon('proof')}<span>证据</span></button>
      </nav>
    </div>
    <div id="drawerLayer" class="drawer-layer" aria-hidden="true"></div>
    <div id="toast" class="toast" role="status"></div>`;

  const workspace = document.getElementById('workspace');
  const drawerLayer = document.getElementById('drawerLayer');
  const toast = document.getElementById('toast');

  const pageHeading = (eyebrow, title, note, tools = '') => `<div class="page-heading"><div><p>${eyebrow}</p><h1>${title}</h1>${note ? `<span>${note}</span>` : ''}</div>${tools}</div>`;

  const metrics = () => `<section class="metrics" aria-label="当前需要关注的事实"><div><span>数据中断</span><strong>4 <small>个空间</small></strong><p>涉及 2 家酒店</p></div><div><span>环境偏离候选</span><strong>7 <small>个空间</small></strong><p>仅与本房间基线比较</p></div><div><span>控制待验证</span><strong>2 <small>笔</small></strong><p>不能显示为现实生效</p></div><div><span>设备身份</span><strong>4 / 10</strong><p>其余 6 台待现场核对</p></div></section>`;

  const lineChart = () => `<section class="chart-panel"><div class="panel-head"><div><h2>需要进入处理的空间</h2><p>过去 24 小时 · 按酒店</p></div><div class="legend"><span><i class="blue"></i>天河</span><span><i class="green"></i>越秀</span><span><i class="gold"></i>千灯湖</span></div></div><svg class="trend-chart" viewBox="0 0 720 220" preserveAspectRatio="none" role="img" aria-label="过去二十四小时三个酒店需要进入处理的空间数量变化"><g class="chart-grid"><path d="M42 24H704M42 78H704M42 132H704M42 186H704"/><path d="M42 24V186M208 24V186M373 24V186M539 24V186M704 24V186"/></g><path class="area" d="M42 142 C88 134 112 120 158 128 S232 150 286 102 S375 75 428 92 S525 122 582 78 S658 62 704 48 L704 186 L42 186Z"/><path class="line blue" d="M42 142 C88 134 112 120 158 128 S232 150 286 102 S375 75 428 92 S525 122 582 78 S658 62 704 48"/><path class="line green" d="M42 158 C102 150 128 160 178 146 S268 122 318 137 S412 153 468 126 S558 112 614 118 S670 103 704 105"/><path class="line gold" d="M42 166 C115 165 147 150 210 158 S310 167 372 150 S474 142 524 151 S626 157 704 136"/><g class="chart-labels"><text x="42" y="210">昨日 10:00</text><text x="355" y="210">22:00</text><text x="653" y="210">今日 09:44</text><text x="13" y="30">12</text><text x="20" y="84">8</text><text x="20" y="138">4</text><text x="20" y="190">0</text></g></svg></section>`;

  const workList = () => `<section class="work-panel"><div class="panel-head"><div><h2>现在需要进入</h2><p>按现实影响与证据中断排序</p></div><span>4 项</span></div><button data-space="1005"><i class="rank red">1</i><span><b>1005 · 数据中断 18 分钟</b><small>历史持续时间已冻结</small></span>${icon('chevron')}</button><button data-view="evidence"><i class="rank amber">2</i><span><b>一笔操作仍待现实验证</b><small>设备已确认执行</small></span>${icon('chevron')}</button><button data-view="work"><i class="rank amber">3</i><span><b>环境偏离房间历史基线</b><small>越秀店 1206 · 22 分钟</small></span>${icon('chevron')}</button><button data-view="devices"><i class="rank blue">4</i><span><b>6 台设备身份待核对</b><small>不编号、不推断位置与状态</small></span>${icon('chevron')}</button></section>`;

  const hotelsTable = () => `<section class="table-panel"><div class="panel-head"><div><h2>酒店对比</h2><p>点击酒店继续进入楼栋、楼层和空间</p></div><button class="link-button" data-view="spaces">进入酒店空间 ${icon('arrow')}</button></div><div class="table-scroll"><table><thead><tr><th>酒店</th><th>可回看空间</th><th>数据中断</th><th>控制能力样本</th><th>待验证</th><th>最近变化</th><th></th></tr></thead><tbody><tr data-view="spaces"><td><i class="hotel-index">1</i><span><b>广州天河店</b><small>GZ-TH</small></span></td><td>46 / 48</td><td class="danger">2 个空间</td><td>31 间</td><td>1 笔</td><td>09:42</td><td>${icon('chevron')}</td></tr><tr data-view="spaces"><td><i class="hotel-index">2</i><span><b>广州越秀店</b><small>GZ-YX</small></span></td><td>38 / 40</td><td class="danger">2 个空间</td><td>26 间</td><td>无</td><td>09:39</td><td>${icon('chevron')}</td></tr><tr data-view="spaces"><td><i class="hotel-index">3</i><span><b>佛山千灯湖店</b><small>FS-QD</small></span></td><td>52 / 52</td><td>无中断</td><td>35 间</td><td>1 笔</td><td>09:44</td><td>${icon('chevron')}</td></tr></tbody></table></div></section>`;

  const spaceCards = () => `<div class="space-grid">${spaces.map((s) => `<button class="space-card state-${s.state}" data-space="${s.no}"><span class="space-no">${s.no}</span><span class="model">${s.model}</span><b>${s.observation}</b><small>${s.env}</small><em>${s.ability}</em></button>`).join('')}</div>`;

  const evidenceSteps = (compact = false) => `<ol class="evidence-steps ${compact ? 'compact' : ''}"><li class="done"><i>1</i><span><b>意图已创建</b><small>09:36:12</small></span></li><li class="done"><i>2</i><span><b>已下发</b><small>09:36:13</small></span></li><li class="done"><i>3</i><span><b>设备确认执行</b><small>09:36:14</small></span></li><li class="current"><i>4</i><span><b>状态已观察</b><small>等待新证据</small></span></li><li><i>5</i><span><b>现实效果已验证</b><small>尚未完成</small></span></li></ol>`;

  function renderOverview() {
    workspace.innerHTML = `${pageHeading('集团态势', '先找到值得进入的酒店', '图表只解释变化与差异，不做无意义综合评分', '<div class="time-tabs"><button>今天</button><button class="active">24 小时</button><button>7 天</button></div>')}${metrics()}<div class="overview-grid">${lineChart()}${workList()}</div>${hotelsTable()}`;
  }

  function renderSpaces() {
    workspace.innerHTML = `${pageHeading('广州天河店 · 1 号楼', '1F 空间', '9 个空间 · 1 个数据中断 · 1 笔控制待验证', '<div class="view-tools"><button class="active">矩阵</button><button>列表</button><button aria-label="筛选">' + icon('filter') + '</button></div>')}<div class="filter-tabs"><button class="active">全部 9</button><button>数据中断 1</button><button>环境偏离 1</button><button>能力样本 4</button><button>身份待核对 1</button></div>${spaceCards()}`;
  }

  function renderWork() {
    workspace.innerHTML = `${pageHeading('工作中心', '只列需要人介入的事项', '问题、影响范围、证据和下一步在同一行', '<button class="outline-button">按酒店筛选 ' + icon('filter') + '</button>')}<div class="work-layout"><section class="issue-list"><header><span>事项</span><span>影响范围</span><span>当前证据</span><span>下一步</span></header><button data-space="1005"><span><i class="signal gap"></i><b>空间观察数据中断</b><small>已持续 18 分钟</small></span><span>天河店 · 1005</span><span>最后样本 09:24:03</span><em>检查节点</em></button><button data-view="evidence"><span><i class="signal verify"></i><b>控制现实效果待确认</b><small>设备已经确认执行</small></span><span>天河店 · 1008</span><span>证据等级 3 / 5</span><em>补充验证</em></button><button><span><i class="signal env"></i><b>环境偏离房间历史基线</b><small>未完成房间环境校准</small></span><span>越秀店 · 1206</span><span>连续 22 分钟</span><em>查看趋势</em></button><button data-view="devices"><span><i class="signal identity"></i><b>设备身份尚未核对</b><small>不推断位置和状态</small></span><span>现场 · 6 台</span><span>无冻结安装记录</span><em>现场核对</em></button></section><aside class="work-summary"><h2>事项构成</h2><div><span>证据中断</span><b>2</b></div><div><span>现实待验证</span><b>2</b></div><div><span>环境偏离候选</span><b>7</b></div><p>这些数字用于定位工作量，不代表酒店健康评分。</p></aside></div>`;
  }

  function renderDevices() {
    workspace.innerHTML = `${pageHeading('节点与设备', '先确认身份，再谈运行与控制', '现场共有 10 台物理设备；当前仅 4 台身份可追溯', '<button class="outline-button">按型号筛选 ' + icon('filter') + '</button>')}<div class="device-layout"><section class="identity-panel"><div class="panel-head"><div><h2>身份账本</h2><p>用户报告的场景不等于冻结安装记录</p></div><strong>4 / 10</strong></div><div class="known-device"><i>1</i><span><b>MS24</b><small>用户报告：前台</small></span><em>活动候选</em></div><div class="known-device"><i>2</i><span><b>LD2450</b><small>用户报告：前台</small></span><em>相对二维</em></div><div class="known-device"><i>3</i><span><b>MS24</b><small>用户报告：办公室</small></span><em>活动候选</em></div><div class="known-device"><i>4</i><span><b>LD2453</b><small>用户报告：办公室</small></span><em>相对二维</em></div><div class="unknown-device"><b>其余 6 台</b><span>型号、位置、身份与运行状态均待核对</span></div></section><section class="table-panel capability"><div class="panel-head"><div><h2>型号能力边界</h2><p>观察能力与执行能力分开表达</p></div></div><div class="table-scroll"><table><thead><tr><th>型号</th><th>已确认</th><th>可以表达</th><th>禁止推断</th><th>执行能力</th></tr></thead><tbody><tr><td><b>LD2450</b></td><td>1</td><td>0 / 1 / 2 / 3+ 候选、相对二维 X/Y</td><td>4 人以上精确人数、姿态、三维</td><td>需独立能力胶囊</td></tr><tr><td><b>LD2453</b></td><td>1</td><td>0 / 1 / 2 / 3+ 候选、相对二维 X/Y</td><td>高度、人体体积、房间平面</td><td>需独立能力胶囊</td></tr><tr><td><b>MS24</b></td><td>2</td><td>活动候选</td><td>人数、二维坐标</td><td>需独立能力胶囊</td></tr><tr class="unknown-row"><td><b>待核对</b></td><td>6</td><td>不推断</td><td>型号、位置、状态</td><td>全部阻断</td></tr></tbody></table></div></section></div>`;
  }

  function renderControl() {
    workspace.innerHTML = `${pageHeading('能力与控制', '先定范围，再决定能否执行', '单控、群控和按型号控制共用同一组逻辑门', '<div class="mode-tabs"><button class="active">单空间</button><button>批量范围</button><button>按型号</button></div>')}<div class="control-layout"><section class="scope-panel"><div class="panel-head"><div><h2>操作范围</h2><p>范围扩大后重新计算所有门</p></div><span>单空间</span></div><div class="scope-tree"><button><b>灵犀示例集团</b><small>3 家酒店</small></button><button class="level-2"><b>广州天河店</b><small>48 个空间</small></button><button class="level-3"><b>1F</b><small>9 个空间</small></button><button class="level-4 selected"><b>1001</b><small>LD2450</small></button></div><button class="outline-button full">切换为批量选择</button></section><section class="gates"><div class="panel-head"><div><h2>执行前检查</h2><p>任何一项不通过，动作不可下发</p></div></div><div class="gate pass"><i>✓</i><span><b>空间与节点已绑定</b><small>1001 · 节点 01</small></span></div><div class="gate pass"><i>✓</i><span><b>能力已匹配且未撤销</b><small>空调电源 · 布局样本</small></span></div><div class="gate pass"><i>✓</i><span><b>操作者权限允许</b><small>门店工程 · 低风险动作</small></span></div><div class="gate hold"><i>!</i><span><b>现实验证方式有限</b><small>不能证明房间温度已经变化</small></span></div></section><section class="actions-panel"><div class="panel-head"><div><h2>可用动作</h2><p>一次只执行一个低风险动作</p></div></div><div class="appliance"><span class="appliance-icon">AC</span><span><b>空调 · 能力样本</b><small>胶囊剩余 18 天 · 未撤销</small></span></div><div class="action-buttons"><button>关闭</button><button class="primary" data-demo-action>开启</button></div><div class="appliance blocked"><span class="appliance-icon">TV</span><span><b>电视</b><small>尚未验证，不能操作</small></span></div><button class="blocked-button" disabled>能力未就绪</button></section><section class="receipt"><div class="panel-head"><div><h2>最近一次操作</h2><p>LX-AE-0821 · 09:36:12</p></div><span>现实待确认</span></div>${evidenceSteps(true)}</section></div><div class="boundary"><b>无人值守自动控制保持关闭</b><span>HTTP 成功、命令排队、红外发波和设备确认都不能显示为现实效果成功。</span></div>`;
  }

  function renderEvidence() {
    workspace.innerHTML = `${pageHeading('证据与运维', '观察与操作使用同一条时间线', '操作进行到哪一级、证据在哪里中断，一眼看清', '<div class="time-tabs"><button>1 小时</button><button class="active">24 小时</button><button>7 天</button></div>')}<div class="evidence-layout"><section class="timeline"><div class="date-label">今天 · 09:00—09:44</div><article class="event gap"><time>09:42</time><i></i><div><span>观察中断</span><h2>天河店 1005 停止产生新证据</h2><p>持续时间已经冻结；最近一条有效样本为 09:24:03。</p><button data-space="1005">进入空间</button></div></article><article class="event control"><time>09:36</time><i></i><div><span>受控操作</span><h2>1008 空调开启：设备确认执行</h2><p>已经完成意图、下发和设备执行；现实效果尚未验证。</p><button data-view="control">查看完整证据</button></div></article><article class="event change"><time>09:31</time><i></i><div><span>空间变化</span><h2>1003 从 1 人候选变为 0 人候选</h2><p>按采样窗口重建，不代表精确离开时间。</p><button data-space="1003">查看当时证据</button></div></article><article class="event env"><time>09:18</time><i></i><div><span>环境变化</span><h2>越秀店 1206 相对本房间基线持续偏离</h2><p>板载温湿度尚未完成房间环境校准。</p><button>进入环境证据</button></div></article></section><aside class="evidence-summary"><div><span>证据缺口</span><strong>4</strong><p>2 个空间数据中断<br>2 笔控制尚未现实验证</p></div><div><span>设备身份</span><strong>4 / 10</strong><p>其余 6 台待现场核对</p></div><div><span>校准边界</span><strong>0</strong><p>无冻结安装段<br>无可信外部环境参考</p></div></aside></div>`;
  }

  const renderers = { overview: renderOverview, spaces: renderSpaces, work: renderWork, devices: renderDevices, control: renderControl, evidence: renderEvidence };

  function setView(view) {
    if (!renderers[view]) return;
    state.view = view;
    document.querySelectorAll('[data-view]').forEach((node) => node.classList.toggle('active', node.dataset.view === view));
    renderers[view]();
    bindWorkspace();
    window.scrollTo({ top: 0, behavior: 'smooth' });
  }

  function openSpace(no) {
    const space = spaces.find((item) => item.no === no) || spaces[0];
    state.selectedSpace = space;
    drawerLayer.innerHTML = `<button class="backdrop" aria-label="关闭空间详情"></button><aside class="drawer"><header><div><small>广州天河店 · 1F</small><h1>${space.no} <span>${space.model}</span></h1></div><button class="close-drawer" aria-label="关闭">${icon('close')}</button></header><nav><button class="active">概览</button><button>历史</button><button>二维活动</button><button>环境</button><button>诊断</button><button>控制</button><button>证据</button></nav><section class="drawer-facts"><div><span>当前观察</span><b>${space.observation}</b><small>证据更新于 ${space.updated}</small></div><div><span>环境证据</span><b>${space.env}</b><small>板载 AHT20 不能代表房间真实温湿度</small></div><div><span>控制能力</span><b>${space.ability}</b><small>只显示未过期、未撤销且与本空间匹配的能力</small></div></section><section class="history"><div><b>最近 1 小时</b><span>按采样窗口重建</span></div><svg viewBox="0 0 480 100" preserveAspectRatio="none"><path class="axis" d="M0 76H480"/><path class="step" d="M0 67H72V53H150V36H236V36H301V60H370V45H438V24H480"/><path class="gap" d="M282 12V82M298 12V82"/><circle cx="72" cy="53" r="4"/><circle cx="150" cy="36" r="4"/><circle cx="301" cy="60" r="4"/><circle cx="438" cy="24" r="4"/></svg><input type="range" min="0" max="100" value="84" aria-label="历史时间位置"><p>09:34 · 活动候选发生变化</p></section><button class="control-entry" data-view="control"><span>${icon('control')}<span><b>进入受控操作</b><small>先检查能力、授权、有效期与验证方式</small></span></span>${icon('chevron')}</button></aside>`;
    drawerLayer.classList.add('open');
    drawerLayer.setAttribute('aria-hidden', 'false');
    document.body.classList.add('no-scroll');
    drawerLayer.querySelector('.backdrop').addEventListener('click', closeDrawer);
    drawerLayer.querySelector('.close-drawer').addEventListener('click', closeDrawer);
    drawerLayer.querySelector('[data-view="control"]').addEventListener('click', () => { closeDrawer(); setView('control'); });
  }

  function closeDrawer() {
    drawerLayer.classList.remove('open');
    drawerLayer.setAttribute('aria-hidden', 'true');
    document.body.classList.remove('no-scroll');
  }

  function bindWorkspace() {
    workspace.querySelectorAll('[data-view]').forEach((node) => node.addEventListener('click', () => setView(node.dataset.view)));
    workspace.querySelectorAll('[data-space]').forEach((node) => node.addEventListener('click', () => openSpace(node.dataset.space)));
    workspace.querySelector('[data-demo-action]')?.addEventListener('click', (event) => {
      const button = event.currentTarget;
      button.disabled = true;
      button.textContent = '意图已创建';
      window.setTimeout(() => { button.disabled = false; button.textContent = '开启'; showToast('仅演示本地交互，没有下发真实命令'); }, 1100);
    });
  }

  function showToast(message) {
    toast.textContent = message;
    toast.classList.add('show');
    window.setTimeout(() => toast.classList.remove('show'), 3000);
  }

  document.querySelectorAll('.main-nav [data-view], .bottom-nav [data-view], .brand[data-view]').forEach((node) => node.addEventListener('click', (event) => { event.preventDefault(); setView(node.dataset.view); }));
  document.querySelector('.mobile-menu').addEventListener('click', () => document.querySelector('.sidebar').classList.toggle('mobile-open'));
  document.addEventListener('keydown', (event) => { if (event.key === 'Escape') closeDrawer(); });
  setView('overview');
})();
