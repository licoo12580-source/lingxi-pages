import assert from "node:assert/strict";
import { access, readFile } from "node:fs/promises";
import test from "node:test";

async function render() {
  const workerUrl = new URL("../dist/server/index.js", import.meta.url);
  workerUrl.searchParams.set("test", `${process.pid}-${Date.now()}`);
  const { default: worker } = await import(workerUrl.href);

  return worker.fetch(
    new Request("http://localhost/", {
      headers: { accept: "text/html" },
    }),
    {
      ASSETS: {
        fetch: async () => new Response("Not found", { status: 404 }),
      },
    },
    {
      waitUntil() {},
      passThroughOnException() {},
    },
  );
}

test("server-renders the six-direction review experience", async () => {
  const response = await render();
  assert.equal(response.status, 200);
  assert.match(response.headers.get("content-type") ?? "", /^text\/html\b/i);

  const html = await response.text();
  assert.match(html, /<title>灵犀空间总控｜六套独立界面方向<\/title>/);
  for (const direction of [
    "房间账本",
    "空间舞台",
    "运营明细",
    "叙事界面",
    "科技拼图",
    "楼层走廊",
  ]) {
    assert.match(html, new RegExp(direction));
  }
  assert.match(html, /隔离样本，不代表现场统计/);
  assert.doesNotMatch(html, /Your site is taking shape|react-loading-skeleton/);
});

test("keeps observation and control claims fail-closed", async () => {
  const [page, layout, css] = await Promise.all([
    readFile(new URL("../app/page.tsx", import.meta.url), "utf8"),
    readFile(new URL("../app/layout.tsx", import.meta.url), "utf8"),
    readFile(new URL("../app/globals.css", import.meta.url), "utf8"),
  ]);

  for (const required of [
    "0 人候选",
    "无法判断",
    "节点离线",
    "传感器故障",
    "活动候选",
    "雷达相对坐标",
    "按采样窗口重建",
    "未下发真实命令",
    "设备确认执行",
    "现实效果验证",
    "批量控制门",
    "逐目标检查",
  ]) {
    assert.match(page, new RegExp(required));
  }

  for (const rejected of [
    "在线且新鲜",
    "待入住",
    "待退房",
    "清扫",
    "维修",
    "预订",
    "PMS",
  ]) {
    assert.doesNotMatch(page, new RegExp(rejected));
    assert.doesNotMatch(layout, new RegExp(rejected));
  }

  assert.match(page, /room\.model === "LD2450" \|\| room\.model === "LD2453"/);
  assert.match(page, /MS24 仅显示活动候选/);
  assert.match(page, /未做房间标定/);
  assert.match(css, /\.state-zero/);
  assert.match(css, /\.state-unknown/);
  assert.match(css, /\.state-offline/);
  assert.match(css, /\.state-fault/);
});

test("does not ship rejected predecessor assets", async () => {
  await assert.rejects(access(new URL("../public/framework.js", import.meta.url)));
  await assert.rejects(access(new URL("../public/framework.css", import.meta.url)));
  await assert.rejects(access(new URL("../app/_sites-preview/SkeletonPreview.tsx", import.meta.url)));
});
