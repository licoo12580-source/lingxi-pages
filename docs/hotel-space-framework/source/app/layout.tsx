import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "灵犀空间总控｜六套独立界面方向",
  description:
    "面向酒店现场的六套独立高保真方向，覆盖空间矩阵、人数与活动历史、雷达相对二维活动、受控操作与证据。",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="zh-CN">
      <body>{children}</body>
    </html>
  );
}
