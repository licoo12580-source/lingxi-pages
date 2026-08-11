"use client";

import { useEffect, useMemo, useState } from "react";

type ObservationKind =
  | "zero"
  | "one"
  | "two"
  | "three"
  | "activity"
  | "gap"
  | "unknown"
  | "offline"
  | "fault";

type Room = {
  id: string;
  kind: ObservationKind;
  observation: string;
  duration: string;
  model: string;
  node: string;
  freshness: string;
  change: string;
  environment: string;
  light: string;
  control: string;
  note: string;
};

type HistoryPoint = {
  time: string;
  kind: ObservationKind;
  label: string;
  note: string;
};

const rooms: Room[] = [
  {
    id: "1001",
    kind: "one",
    observation: "1 人候选",
    duration: "持续 12 分钟",
    model: "LD2450",
    node: "节点 02",
    freshness: "刚刚更新",
    change: "09:31 从 0 变为 1",
    environment: "板内 31.6°C / 48%RH · 未校准",
    light: "相对微亮",
    control: "空调 · 1 项试配能力",
    note: "当前无待处理事项",
  },
  {
    id: "1002",
    kind: "activity",
    observation: "活动候选",
    duration: "持续 6 分钟",
    model: "MS24",
    node: "节点 01",
    freshness: "1 分钟前",
    change: "09:37 活动恢复",
    environment: "板内 30.9°C / 51%RH · 未校准",
    light: "相对明亮",
    control: "未匹配控制能力",
    note: "MS24 不提供人数与二维坐标",
  },
  {
    id: "1003",
    kind: "zero",
    observation: "0 人候选",
    duration: "持续 28 分钟",
    model: "LD2453",
    node: "节点 04",
    freshness: "刚刚更新",
    change: "09:15 从 1 变为 0",
    environment: "板内 29.8°C / 53%RH · 未校准",
    light: "相对偏暗",
    control: "电视 · 1 项试配能力",
    note: "仅为多帧有界候选，不代表空房事实",
  },
  {
    id: "1005",
    kind: "gap",
    observation: "数据中断",
    duration: "18 分钟",
    model: "LD2450",
    node: "节点 05",
    freshness: "最后样本 09:24",
    change: "09:24 证据停止",
    environment: "环境证据同时中断",
    light: "无法判断",
    control: "全部控制已阻断",
    note: "持续时间冻结，优先检查节点",
  },
  {
    id: "1006",
    kind: "three",
    observation: "3+ 人候选",
    duration: "持续 3 分钟",
    model: "LD2453",
    node: "节点 06",
    freshness: "刚刚更新",
    change: "09:40 从 2 变为 3+",
    environment: "板内 32.1°C / 47%RH · 未校准",
    light: "相对变亮",
    control: "未匹配控制能力",
    note: "硬件最多 3 个目标槽，不声明精确人数",
  },
  {
    id: "1007",
    kind: "unknown",
    observation: "无法判断",
    duration: "身份待核对",
    model: "型号待核对",
    node: "节点待绑定",
    freshness: "无当前证据",
    change: "—",
    environment: "无可追溯来源",
    light: "无法判断",
    control: "全部控制已阻断",
    note: "不自动编号，不推断位置与运行状态",
  },
  {
    id: "1008",
    kind: "two",
    observation: "2 人候选",
    duration: "持续 4 分钟",
    model: "LD2450",
    node: "节点 08",
    freshness: "刚刚更新",
    change: "09:39 从 1 变为 2",
    environment: "板内 31.2°C / 49%RH · 未校准",
    light: "相对微亮",
    control: "空调 · 低风险单动作样本",
    note: "一笔操作等待独立现实验证",
  },
  {
    id: "1009",
    kind: "offline",
    observation: "节点离线",
    duration: "持续 31 分钟",
    model: "MS24",
    node: "节点 09",
    freshness: "最后接收 09:11",
    change: "09:11 节点离线",
    environment: "证据中断",
    light: "无法判断",
    control: "全部控制已阻断",
    note: "离线不等于 0 人，也不等于传感器故障",
  },
  {
    id: "1010",
    kind: "fault",
    observation: "传感器故障",
    duration: "连续 7 个无效窗口",
    model: "LD2453",
    node: "节点 10",
    freshness: "节点仍有心跳",
    change: "09:36 雷达帧失效",
    environment: "板内数据仍有更新",
    light: "相对微亮",
    control: "观察门阻断",
    note: "连接仍在，雷达观察不可用",
  },
];

const history: HistoryPoint[] = [
  { time: "08:44", kind: "zero", label: "0 人候选", note: "多帧窗口重建" },
  { time: "08:56", kind: "one", label: "1 人候选", note: "目标 A 首次出现" },
  { time: "09:08", kind: "one", label: "1 人候选", note: "低速移动" },
  { time: "09:17", kind: "gap", label: "数据空窗", note: "缺少 2 个采样窗口" },
  { time: "09:24", kind: "one", label: "1 人候选", note: "证据恢复" },
  { time: "09:31", kind: "two", label: "2 人候选", note: "目标 B 出现" },
  { time: "09:39", kind: "three", label: "3+ 人候选", note: "有界候选上限" },
  { time: "09:44", kind: "two", label: "2 人候选", note: "当前窗口" },
];

const directionMeta = [
  { id: 1, name: "房间账本", note: "紧凑矩阵 + 固定检查面板" },
  { id: 2, name: "空间舞台", note: "二维画布 + 房间轨道" },
  { id: 3, name: "运营明细", note: "表格主导 + 行内穿透" },
  { id: 4, name: "叙事界面", note: "大字层级 + 变化故事" },
  { id: 5, name: "科技拼图", note: "柔和模块 + 多窗口协同" },
  { id: 6, name: "楼层走廊", note: "物理层级 + 底部工作台" },
];

const kindLabel: Record<ObservationKind, string> = {
  zero: "0",
  one: "1",
  two: "2",
  three: "3+",
  activity: "≈",
  gap: "∥",
  unknown: "?",
  offline: "×",
  fault: "!",
};

function StateMark({
  kind,
  label,
  compact = false,
}: {
  kind: ObservationKind;
  label?: string;
  compact?: boolean;
}) {
  return (
    <span className={"state-mark state-" + kind + (compact ? " is-compact" : "")}>
      <i aria-hidden="true">{kindLabel[kind]}</i>
      {label ? <b>{label}</b> : null}
    </span>
  );
}

function ReviewDock({
  direction,
  onDirection,
}: {
  direction: number;
  onDirection: (value: number) => void;
}) {
  return (
    <header className="review-dock">
      <div className="review-title">
        <span>视觉评审</span>
        <b>六套结构方向</b>
        <small>隔离样本，不代表现场统计</small>
      </div>
      <div className="review-options" role="tablist" aria-label="选择布局方向">
        {directionMeta.map((item) => (
          <button
            type="button"
            role="tab"
            aria-selected={direction === item.id}
            className={direction === item.id ? "active" : ""}
            key={item.id}
            onClick={() => onDirection(item.id)}
          >
            <i>{String(item.id).padStart(2, "0")}</i>
            <span>{item.name}</span>
          </button>
        ))}
      </div>
      <div className="review-current">
        <b>{directionMeta[direction - 1].name}</b>
        <span>{directionMeta[direction - 1].note}</span>
      </div>
    </header>
  );
}

function ScopeLine({ compact = false }: { compact?: boolean }) {
  return (
    <div className={"scope-line" + (compact ? " compact" : "")}>
      <button type="button">评审集团 <span>⌄</span></button>
      <i aria-hidden="true">/</i>
      <button type="button">广州样本酒店 <span>⌄</span></button>
      <i aria-hidden="true">/</i>
      <button type="button">1 号楼 · 1F <span>⌄</span></button>
      <label>
        <span aria-hidden="true">⌕</span>
        <input aria-label="搜索房号、空间或设备" placeholder="搜索房号、空间或设备" />
      </label>
    </div>
  );
}

function PeopleLegend() {
  return (
    <div className="people-legend" aria-label="人数和观察状态说明">
      {[
        ["zero", "0 人候选"],
        ["one", "1 人候选"],
        ["two", "2 人候选"],
        ["three", "3+ 候选"],
        ["activity", "活动候选"],
        ["unknown", "无法判断"],
      ].map(([kind, label]) => (
        <StateMark key={kind} kind={kind as ObservationKind} label={label} compact />
      ))}
    </div>
  );
}

function FilterLine() {
  return (
    <div className="filter-line" aria-label="空间筛选">
      <button className="active" type="button">全部空间 <b>9</b></button>
      <button type="button">数据中断 <b>1</b></button>
      <button type="button">节点离线 <b>1</b></button>
      <button type="button">传感器故障 <b>1</b></button>
      <button type="button">可控能力 <b>3</b></button>
    </div>
  );
}

function RoomCard({
  room,
  selected,
  onSelect,
  style = "card",
}: {
  room: Room;
  selected: boolean;
  onSelect: (room: Room) => void;
  style?: "card" | "strip" | "door" | "mini";
}) {
  return (
    <button
      type="button"
      className={
        "room-unit room-" +
        style +
        " room-kind-" +
        room.kind +
        (selected ? " selected" : "")
      }
      onClick={() => onSelect(room)}
      aria-pressed={selected}
    >
      <span className="room-unit-top">
        <strong>{room.id}</strong>
        <StateMark kind={room.kind} />
      </span>
      <b className="room-observation">{room.observation}</b>
      <span className="room-duration">{room.duration}</span>
      {style !== "mini" && style !== "door" ? (
        <>
          <small>{room.model} · {room.freshness}</small>
          <em>{room.note}</em>
        </>
      ) : null}
    </button>
  );
}

function StatusSummary() {
  return (
    <div className="status-summary">
      <div><span>需要先处理</span><b>3</b><small>中断、离线、故障分开</small></div>
      <div><span>当前有界候选</span><b>5</b><small>不包含 MS24 活动候选</small></div>
      <div><span>控制待验证</span><b>1</b><small>设备确认不等于现实生效</small></div>
      <div><span>可批量检查</span><b>2</b><small>同型号且能力已匹配</small></div>
    </div>
  );
}

function PersonFigure({
  id,
  ghost = false,
  unstable = false,
  position,
}: {
  id: string;
  ghost?: boolean;
  unstable?: boolean;
  position: string;
}) {
  return (
    <div
      className={
        "person-figure person-" +
        position +
        (ghost ? " ghost" : "") +
        (unstable ? " unstable" : "")
      }
      aria-label={(ghost ? "历史目标 " : "当前目标 ") + id + (unstable ? "，关联不稳定" : "")}
    >
      <span className="person-shape" aria-hidden="true">
        <i className="person-head" />
        <i className="person-body" />
        <i className="person-arm arm-left" />
        <i className="person-arm arm-right" />
        <i className="person-leg leg-left" />
        <i className="person-leg leg-right" />
      </span>
      <b>{id}</b>
    </div>
  );
}

function SpatialView({
  room,
  historyIndex,
  mode = "full",
}: {
  room: Room;
  historyIndex: number;
  mode?: "full" | "compact" | "wide";
}) {
  const point = history[historyIndex];
  const kind = room.kind === "two" ? point.kind : room.kind;
  const supportsSpatial = room.model === "LD2450" || room.model === "LD2453";
  const unavailable =
    !supportsSpatial ||
    ["gap", "unknown", "offline", "fault"].includes(kind);
  const personCount =
    kind === "one" ? 1 : kind === "two" ? 2 : kind === "three" ? 3 : 0;

  return (
    <section className={"spatial-view spatial-mode-" + mode}>
      <header>
        <div>
          <span>相对二维活动</span>
          <b>{room.id} · {room.model}</b>
        </div>
        <em>雷达相对坐标 · 未做房间标定</em>
      </header>
      <div className={"spatial-field field-" + kind}>
        <div className="coordinate-axis axis-x"><span>左</span><b>设备原点</b><span>右</span></div>
        <div className="coordinate-axis axis-y"><span>远</span><span>近</span></div>
        <div className="radar-fov" aria-hidden="true" />
        <div className="radar-device" aria-label={"灵犀节点 " + room.node + "，朝向画布上方"}>
          <span className="radar-rays" aria-hidden="true"><i /><i /><i /></span>
          <span className="radar-core" aria-hidden="true"><i /><i /><i /></span>
          <b>{room.node}</b>
          <small>朝向 ↑</small>
        </div>
        {!unavailable ? (
          <>
            <div className="heat-shape heat-a"><span>活动轮廓</span></div>
            {personCount > 1 ? <div className="heat-shape heat-b" /> : null}
            <div className="trail trail-a" aria-label="目标 A 连续移动轨迹">
              <i /><i /><i /><i />
            </div>
            {personCount > 1 ? (
              <div className="trail trail-b" aria-label="目标 B 连续移动轨迹">
                <i /><i /><i />
              </div>
            ) : null}
            <PersonFigure id="A" position={"a" + (historyIndex % 3)} />
            <PersonFigure id="A" position="a-ghost" ghost />
            {personCount > 1 ? <PersonFigure id="B" position={"b" + (historyIndex % 2)} /> : null}
            {personCount > 2 ? <PersonFigure id="C" position="c0" unstable /> : null}
          </>
        ) : (
          <div className="spatial-unavailable">
            <StateMark kind={kind} />
            <b>
              {!supportsSpatial
                ? "该型号不提供二维坐标"
                : kind === "gap"
                  ? "数据空窗，轨迹冻结"
                  : kind === "offline"
                    ? "节点离线，无法继续观察"
                    : kind === "fault"
                      ? "雷达观察故障"
                      : "当前无法形成二维候选"}
            </b>
            <span>
              {!supportsSpatial
                ? "MS24 仅显示活动候选"
                : "不使用旧坐标冒充当前位置"}
            </span>
          </div>
        )}
        {!unavailable && personCount === 0 ? (
          <div className="zero-window"><StateMark kind="zero" /><b>当前窗口无目标候选</b><span>不等于真实空房</span></div>
        ) : null}
      </div>
      <footer>
        <span><i className="legend-device" /> 灵犀设备</span>
        <span><i className="legend-person" /> 当前人形</span>
        <span><i className="legend-ghost" /> 历史残影</span>
        <span><i className="legend-trail" /> 连续尾迹</span>
        <span><i className="legend-heat" /> 活动轮廓</span>
      </footer>
    </section>
  );
}

function HistoryControl({
  index,
  onIndex,
  playing,
  onPlaying,
  style = "panel",
}: {
  index: number;
  onIndex: (value: number) => void;
  playing: boolean;
  onPlaying: (value: boolean) => void;
  style?: "panel" | "rail" | "editorial";
}) {
  const point = history[index];
  const [windowRange, setWindowRange] = useState("1 小时");
  return (
    <section className={"history-control history-" + style}>
      <header>
        <div>
          <span>人数 / 活动历史</span>
          <b>{point.time} · {point.label}</b>
        </div>
        <div className="window-tabs" aria-label="历史范围">
          {["1 小时", "6 小时", "24 小时"].map((range) => (
            <button
              className={windowRange === range ? "active" : ""}
              type="button"
              key={range}
              onClick={() => setWindowRange(range)}
            >
              {range}
            </button>
          ))}
        </div>
      </header>
      <div className="history-body">
        <div className="play-controls">
          <button type="button" aria-label="向前跳转" onClick={() => onIndex(Math.max(0, index - 1))}>‹</button>
          <button
            className="play"
            type="button"
            aria-label={playing ? "暂停历史播放" : "播放历史"}
            onClick={() => onPlaying(!playing)}
          >
            {playing ? "Ⅱ" : "▶"}
          </button>
          <button type="button" aria-label="向后跳转" onClick={() => onIndex(Math.min(history.length - 1, index + 1))}>›</button>
        </div>
        <div className="step-track" aria-hidden="true">
          {history.map((item, itemIndex) => (
            <i
              key={item.time}
              className={
                "step-segment segment-" +
                item.kind +
                (itemIndex === index ? " current" : "")
              }
            >
              <span>{kindLabel[item.kind]}</span>
            </i>
          ))}
        </div>
        <input
          aria-label="拖动回看人数和活动历史"
          type="range"
          min="0"
          max={history.length - 1}
          step="1"
          value={index}
          onChange={(event) => onIndex(Number(event.target.value))}
          onInput={(event) => onIndex(Number(event.currentTarget.value))}
        />
        <div className="history-times">
          <span>{history[0].time}</span>
          <b>{point.note} · 按采样窗口重建</b>
          <span>{history[history.length - 1].time}</span>
        </div>
      </div>
    </section>
  );
}

function EvidenceRail({ compact = false }: { compact?: boolean }) {
  const stages = [
    ["意图已创建", "09:36:12", "done"],
    ["已经下发", "09:36:13", "done"],
    ["设备确认执行", "09:36:14", "done"],
    ["状态被观察", "等待新证据", "current"],
    ["现实效果验证", "未验证，不可称成功", "pending"],
    ["纠错或归档", "等待结果", "pending"],
  ];
  return (
    <ol className={"evidence-rail" + (compact ? " compact" : "")}>
      {stages.map(([label, note, state], index) => (
        <li className={state} key={label}>
          <i>{index + 1}</i>
          <span><b>{label}</b><small>{note}</small></span>
        </li>
      ))}
    </ol>
  );
}

function BatchControlBar() {
  const [selected, setSelected] = useState(false);
  const [checked, setChecked] = useState(false);

  const toggleSelection = () => {
    setSelected((current) => !current);
    setChecked(false);
  };

  return (
    <section className="batch-control-bar" aria-label="批量控制门样本">
      <div>
        <span>批量控制门</span>
        <b>按设备型号与能力胶囊筛选</b>
        <small>批量范围不会继承单房授权或验证结果</small>
      </div>
      <ul aria-label="批量样本范围">
        <li><b>{selected ? "2" : "0"}</b><span>已选空间</span></li>
        <li><b>LD2450</b><span>型号筛选</span></li>
        <li><b>空调</b><span>能力类型</span></li>
        <li><b>逐目标</b><span>G5 / G6 / G8</span></li>
      </ul>
      <div className="batch-actions">
        <button type="button" onClick={toggleSelection}>
          {selected ? "清除样本范围" : "选择兼容样本"}
        </button>
        <button type="button" disabled={!selected || checked} onClick={() => setChecked(true)}>
          {checked ? "检查单已创建 · 未下发真实命令" : "逐目标检查"}
        </button>
      </div>
    </section>
  );
}

function RoomFacts({ room }: { room: Room }) {
  return (
    <div className="room-facts">
      <div><span>当前观察</span><b>{room.observation}</b><small>{room.duration} · {room.freshness}</small></div>
      <div><span>最近变化</span><b>{room.change}</b><small>候选变化不代表精确进出时间</small></div>
      <div><span>环境</span><b>{room.environment}</b><small>{room.light} · board_local</small></div>
      <div><span>能力</span><b>{room.control}</b><small>按能力、授权、TTL 与验证门启用</small></div>
    </div>
  );
}

function ControlCard({
  created,
  onCreate,
  compact = false,
}: {
  created: boolean;
  onCreate: () => void;
  compact?: boolean;
}) {
  return (
    <section className={"control-card" + (compact ? " compact" : "")}>
      <header>
        <div><span>受控操作样本</span><b>空调 · 开机</b></div>
        <em>低风险单动作</em>
      </header>
      <div className="gate-row"><i>G5</i><span><b>人工授权</b><small>本次视觉样本内有效</small></span><strong>通过</strong></div>
      <div className="gate-row"><i>G6</i><span><b>能力与有效期</b><small>胶囊样本尚未撤销</small></span><strong>通过</strong></div>
      <div className="gate-row hold"><i>G8</i><span><b>现实验证</b><small>缺少独立效果证据</small></span><strong>等待</strong></div>
      <button type="button" onClick={onCreate} disabled={created}>
        {created ? "演示意图已创建 · 未下发真实命令" : "创建演示意图"}
      </button>
      <p>批量控制必须重新核对每个目标的能力、授权和验证方式，不能继承单房结果。</p>
    </section>
  );
}

function RackDirection(props: DirectionProps) {
  const { selected, onSelect, historyIndex, onHistory, playing, onPlaying, intentCreated, onIntent } = props;
  return (
    <div className="direction dir-rack">
      <aside className="rack-nav">
        <div className="brand-block"><i>灵</i><span><b>灵犀</b><small>酒店空间总控</small></span></div>
        {["集团态势", "酒店空间", "工作中心", "节点与设备", "能力与控制", "证据与运维"].map((item, index) => (
          <button className={index === 1 ? "active" : ""} type="button" key={item}><i>0{index + 1}</i><span>{item}</span></button>
        ))}
        <div className="automation-state"><span>无人值守控制</span><b>关闭</b></div>
      </aside>
      <main className="rack-main">
        <ScopeLine />
        <section className="rack-heading">
          <div><span>广州样本酒店 · 1 号楼</span><h1>1F 空间账本</h1><p>先扫完整楼层，再固定检查一个空间；卡片不会因数据更新频繁跳位。</p></div>
          <PeopleLegend />
        </section>
        <FilterLine />
        <div className="rack-work">
          <section className="room-rack" aria-label="空间矩阵">
            {rooms.map((room) => <RoomCard key={room.id} room={room} selected={selected.id === room.id} onSelect={onSelect} />)}
          </section>
          <aside className="fixed-inspector">
            <header><span>当前检查</span><h2>{selected.id}</h2><StateMark kind={selected.kind} label={selected.observation} compact /></header>
            <RoomFacts room={selected} />
            <SpatialView room={selected} historyIndex={historyIndex} mode="compact" />
            <HistoryControl index={historyIndex} onIndex={onHistory} playing={playing} onPlaying={onPlaying} style="rail" />
            <details>
              <summary>查看控制证据</summary>
              <EvidenceRail compact />
              <ControlCard created={intentCreated} onCreate={onIntent} compact />
            </details>
          </aside>
        </div>
      </main>
    </div>
  );
}

function StageDirection(props: DirectionProps) {
  const { selected, onSelect, historyIndex, onHistory, playing, onPlaying, intentCreated, onIntent } = props;
  return (
    <div className="direction dir-stage">
      <header className="stage-top">
        <div className="stage-brand"><i>灵</i><b>空间舞台</b><span>观察、回看、控制证据</span></div>
        <ScopeLine compact />
        <nav>{["态势", "空间", "设备", "控制", "证据"].map((item, index) => <button className={index === 1 ? "active" : ""} type="button" key={item}>{item}</button>)}</nav>
      </header>
      <main className="stage-main">
        <aside className="stage-rooms">
          <header><span>1F · 9 个空间</span><b>选择空间</b></header>
          {rooms.map((room) => <RoomCard key={room.id} room={room} selected={selected.id === room.id} onSelect={onSelect} style="strip" />)}
        </aside>
        <section className="stage-canvas">
          <SpatialView room={selected} historyIndex={historyIndex} mode="wide" />
          <HistoryControl index={historyIndex} onIndex={onHistory} playing={playing} onPlaying={onPlaying} />
        </section>
        <aside className="stage-insight">
          <header><span>空间 {selected.id}</span><StateMark kind={selected.kind} label={selected.observation} /></header>
          <RoomFacts room={selected} />
          <section className="target-list">
            <div><StateMark kind="one" /><span><b>目标 A</b><small>首次出现 09:31 · 轨迹连续</small></span><em>0.34 m/s</em></div>
            <div><StateMark kind="two" /><span><b>目标 B</b><small>首次出现 09:39 · 关联一般</small></span><em>0.12 m/s</em></div>
          </section>
          <EvidenceRail compact />
          <ControlCard created={intentCreated} onCreate={onIntent} compact />
        </aside>
      </main>
    </div>
  );
}

function LedgerDirection(props: DirectionProps) {
  const { selected, onSelect, historyIndex, onHistory, playing, onPlaying, intentCreated, onIntent } = props;
  return (
    <div className="direction dir-ledger">
      <header className="ledger-top">
        <div><b>LINGXI</b><span>酒店空间总控</span></div>
        <nav>{["集团", "酒店空间", "工作", "设备", "控制", "证据"].map((item, index) => <button className={index === 1 ? "active" : ""} type="button" key={item}>{item}</button>)}</nav>
        <button className="ledger-search" type="button">搜索 ⌕</button>
      </header>
      <main className="ledger-main">
        <ScopeLine compact />
        <div className="ledger-title">
          <div><span>楼层明细 / 1F</span><h1>每一行都是一个可穿透空间</h1></div>
          <StatusSummary />
        </div>
        <FilterLine />
        <BatchControlBar />
        <div className="ledger-table-wrap">
          <table className="ledger-table">
            <thead><tr><th>空间</th><th>当前候选</th><th>状态开始</th><th>最近变化</th><th>证据更新</th><th>环境来源</th><th>控制能力</th><th>下一步</th></tr></thead>
            <tbody>
              {rooms.map((room) => (
                <tr key={room.id} className={selected.id === room.id ? "selected" : ""} onClick={() => onSelect(room)}>
                  <td><b>{room.id}</b><small>{room.node} · {room.model}</small></td>
                  <td><StateMark kind={room.kind} label={room.observation} compact /></td>
                  <td>{room.duration}</td>
                  <td>{room.change}</td>
                  <td>{room.freshness}</td>
                  <td>{room.environment}</td>
                  <td>{room.control}</td>
                  <td><button type="button" onClick={(event) => { event.stopPropagation(); onSelect(room); }}>展开</button></td>
                </tr>
              ))}
            </tbody>
          </table>
          <div className="ledger-mobile">
            {rooms.map((room) => <RoomCard key={room.id} room={room} selected={selected.id === room.id} onSelect={onSelect} style="strip" />)}
          </div>
        </div>
        <section className="ledger-detail">
          <header><div><span>选中空间</span><h2>{selected.id}</h2></div><StateMark kind={selected.kind} label={selected.observation} /></header>
          <div className="ledger-detail-grid">
            <SpatialView room={selected} historyIndex={historyIndex} mode="wide" />
            <HistoryControl index={historyIndex} onIndex={onHistory} playing={playing} onPlaying={onPlaying} />
            <div className="ledger-side">
              <RoomFacts room={selected} />
              <EvidenceRail compact />
              <ControlCard created={intentCreated} onCreate={onIntent} compact />
            </div>
          </div>
        </section>
      </main>
    </div>
  );
}

function EditorialDirection(props: DirectionProps) {
  const { selected, onSelect, historyIndex, onHistory, playing, onPlaying, intentCreated, onIntent } = props;
  return (
    <div className="direction dir-editorial">
      <header className="editorial-top">
        <b>灵犀 / 酒店空间总控</b>
        <ScopeLine compact />
        <nav><button type="button">空间</button><button type="button">设备</button><button type="button">控制</button></nav>
      </header>
      <main className="editorial-main">
        <section className="editorial-hero">
          <span>广州样本酒店 · 1F · 09:44</span>
          <h1>九个空间，<br /><em>三个需要先处理。</em></h1>
          <p>数据中断、节点离线与传感器故障分别处理。0 人候选、未知和离线不共用一种样式。</p>
        </section>
        <section className="editorial-rooms">
          <header><b>空间扫描</b><span>点击任一空间进入同页叙事</span></header>
          <div>
            {rooms.map((room) => (
              <button className={selected.id === room.id ? "selected" : ""} type="button" key={room.id} onClick={() => onSelect(room)}>
                <span>{room.id}</span><StateMark kind={room.kind} /><b>{room.observation}</b><small>{room.duration}</small>
              </button>
            ))}
          </div>
        </section>
        <section className="editorial-story">
          <div className="story-copy">
            <span>当前空间 / {selected.id}</span>
            <h2>{selected.observation}</h2>
            <p>{selected.note}</p>
            <dl>
              <div><dt>从何时开始</dt><dd>{selected.duration}</dd></div>
              <div><dt>最近变化</dt><dd>{selected.change}</dd></div>
              <div><dt>证据状态</dt><dd>{selected.freshness}</dd></div>
              <div><dt>环境边界</dt><dd>{selected.environment}</dd></div>
            </dl>
          </div>
          <SpatialView room={selected} historyIndex={historyIndex} mode="full" />
          <HistoryControl index={historyIndex} onIndex={onHistory} playing={playing} onPlaying={onPlaying} style="editorial" />
          <div className="story-control">
            <h3>操作没有结束在“已发送”</h3>
            <EvidenceRail />
            <ControlCard created={intentCreated} onCreate={onIntent} compact />
          </div>
        </section>
      </main>
    </div>
  );
}

function BentoDirection(props: DirectionProps) {
  const { selected, onSelect, historyIndex, onHistory, playing, onPlaying, intentCreated, onIntent } = props;
  return (
    <div className="direction dir-bento">
      <aside className="bento-rail">
        <i>灵</i>
        {["总", "空", "工", "设", "控", "证"].map((item, index) => <button className={index === 1 ? "active" : ""} type="button" key={item}>{item}</button>)}
        <span>自动<br /><b>关闭</b></span>
      </aside>
      <main className="bento-main">
        <header className="bento-top">
          <div><span>酒店空间</span><h1>广州样本酒店 · 1F</h1></div>
          <ScopeLine compact />
        </header>
        <section className="bento-grid">
          <article className="bento-status">
            <span>当前选择</span><h2>{selected.id}</h2><StateMark kind={selected.kind} label={selected.observation} /><p>{selected.note}</p>
          </article>
          <article className="bento-spaces">
            <header><div><span>空间矩阵</span><b>9 个固定位置</b></div><PeopleLegend /></header>
            <div>{rooms.map((room) => <RoomCard key={room.id} room={room} selected={selected.id === room.id} onSelect={onSelect} style="mini" />)}</div>
          </article>
          <article className="bento-map"><SpatialView room={selected} historyIndex={historyIndex} mode="wide" /></article>
          <article className="bento-history"><HistoryControl index={historyIndex} onIndex={onHistory} playing={playing} onPlaying={onPlaying} /></article>
          <article className="bento-evidence">
            <header><span>证据进度</span><b>3 / 6</b></header>
            <EvidenceRail compact />
          </article>
          <article className="bento-control"><ControlCard created={intentCreated} onCreate={onIntent} compact /></article>
          <article className="bento-boundary">
            <span>环境边界</span><b>board_local</b><p>{selected.environment}</p><small>未完成房间环境校准</small>
          </article>
        </section>
      </main>
    </div>
  );
}

function CorridorDirection(props: DirectionProps) {
  const { selected, onSelect, historyIndex, onHistory, playing, onPlaying, intentCreated, onIntent } = props;
  return (
    <div className="direction dir-corridor">
      <header className="corridor-top">
        <div className="corridor-brand"><i>灵</i><span><b>酒店空间总控</b><small>楼层工作台</small></span></div>
        <ScopeLine />
        <nav>{["集团", "空间", "工作", "设备", "控制", "证据"].map((item, index) => <button className={index === 1 ? "active" : ""} type="button" key={item}>{item}</button>)}</nav>
      </header>
      <main className="corridor-main">
        <section className="corridor-heading">
          <div><span>1 号楼</span><h1>1F</h1><p>沿楼层方向找到空间，再在底部工作台查看观察、历史和控制证据。</p></div>
          <StatusSummary />
        </section>
        <section className="corridor-map" aria-label="楼层空间导航样本">
          <div className="corridor-side north">
            {rooms.slice(0, 5).map((room) => <RoomCard key={room.id} room={room} selected={selected.id === room.id} onSelect={onSelect} style="door" />)}
          </div>
          <div className="corridor-path">
            <span>西侧电梯</span><i /><b>1F 主通道 · 布局样本</b><i /><span>东侧楼梯</span>
          </div>
          <div className="corridor-side south">
            {rooms.slice(5).map((room) => <RoomCard key={room.id} room={room} selected={selected.id === room.id} onSelect={onSelect} style="door" />)}
          </div>
        </section>
        <section className="corridor-workbench">
          <header><div><span>空间工作台</span><h2>{selected.id}</h2></div><StateMark kind={selected.kind} label={selected.observation} /></header>
          <div className="corridor-work-grid">
            <RoomFacts room={selected} />
            <SpatialView room={selected} historyIndex={historyIndex} mode="wide" />
            <HistoryControl index={historyIndex} onIndex={onHistory} playing={playing} onPlaying={onPlaying} />
            <div className="corridor-control"><EvidenceRail compact /><ControlCard created={intentCreated} onCreate={onIntent} compact /></div>
          </div>
        </section>
      </main>
    </div>
  );
}

type DirectionProps = {
  selected: Room;
  onSelect: (room: Room) => void;
  historyIndex: number;
  onHistory: (value: number) => void;
  playing: boolean;
  onPlaying: (value: boolean) => void;
  intentCreated: boolean;
  onIntent: () => void;
};

export default function Home() {
  const [direction, setDirection] = useState(1);
  const [selectedId, setSelectedId] = useState("1008");
  const [historyIndex, setHistoryIndex] = useState(history.length - 1);
  const [playing, setPlaying] = useState(false);
  const [intentCreated, setIntentCreated] = useState(false);

  const selected = useMemo(
    () => rooms.find((room) => room.id === selectedId) ?? rooms[0],
    [selectedId],
  );

  useEffect(() => {
    const params = new URLSearchParams(window.location.search);
    const requested = Number(params.get("v"));
    if (requested >= 1 && requested <= directionMeta.length) {
      const frame = window.requestAnimationFrame(() => setDirection(requested));
      return () => window.cancelAnimationFrame(frame);
    }
  }, []);

  useEffect(() => {
    if (!playing) return;
    const timer = window.setInterval(() => {
      setHistoryIndex((current) => {
        if (current >= history.length - 1) {
          setPlaying(false);
          return current;
        }
        return current + 1;
      });
    }, 900);
    return () => window.clearInterval(timer);
  }, [playing]);

  const changeDirection = (value: number) => {
    setDirection(value);
    const url = new URL(window.location.href);
    url.searchParams.set("v", String(value));
    window.history.replaceState({}, "", url);
    window.scrollTo({ top: 0, behavior: "smooth" });
  };

  const commonProps: DirectionProps = {
    selected,
    onSelect: (room) => {
      setSelectedId(room.id);
      setIntentCreated(false);
    },
    historyIndex,
    onHistory: (value) => {
      setHistoryIndex(value);
      setPlaying(false);
    },
    playing,
    onPlaying: setPlaying,
    intentCreated,
    onIntent: () => setIntentCreated(true),
  };

  return (
    <div className={"prototype prototype-v" + direction}>
      <ReviewDock direction={direction} onDirection={changeDirection} />
      {direction === 1 ? <RackDirection {...commonProps} /> : null}
      {direction === 2 ? <StageDirection {...commonProps} /> : null}
      {direction === 3 ? <LedgerDirection {...commonProps} /> : null}
      {direction === 4 ? <EditorialDirection {...commonProps} /> : null}
      {direction === 5 ? <BentoDirection {...commonProps} /> : null}
      {direction === 6 ? <CorridorDirection {...commonProps} /> : null}
    </div>
  );
}
