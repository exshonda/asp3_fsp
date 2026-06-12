# CLAUDE.md — asp3_fsp

TOPPERS/ASP3 Core を Renesas FSP（RAファミリ）と協調動作させる **SDK統合リポジトリ**。
純カーネル（`asp3_core`）を submodule 参照し、FSP 固有部だけを本リポジトリに持つ。

> 設計・経緯の正本は submodule 側 `asp3/asp3_core/docs/dev/fsp-integration.md`。
> カーネル本体の規約は `asp3/asp3_core/AGENTS.md`。

---

## 0. リポジトリ構成

```
asp3_fsp/
├── asp3/
│   ├── asp3_fsp.cmake               ← 協調ヘルパ（ASP3_TARGET/ASP3_TARGET_DIR 解決）
│   ├── asp3_core/                   ← submodule（純カーネル＋全アーキ/チップ依存部）※public
│   └── target/                      ← FSP ターゲット依存部（ek_ra6m5・ek_ra8m2）
├── ek_ra6m5/sample/                 ← EK-RA6M5 向けボード/アプリ
└── ek_ra8m2/sample/                 ← EK-RA8M2 向けボード/アプリ
└── .claude/skills/porting-asp3-to-renesas-ra/  ← 移植ガイドskill
```

- ビルドは `asp3_core` の正準 CMakeLists をライブラリ専用モード（`ASP3_LIBRARY_ONLY`）で
  取り込み、FSP 固有の arch/target は `ASP3_TARGET_DIR` 方式で外部供給する。
- ツールチェーンは **ARM LLVM Toolchain（clang）**、構成生成は **RASC（FSP 6.2.0 同梱版）**。

## 1. ⚠️ 禁則（作業前に必読）

1. **`asp3/asp3_core/`（submodule）配下を直接編集しない**。カーネル本体は上流 ASP3 追従領域。
   変更が必要なら asp3_core リポジトリ側で行い、その `AGENTS.md` の規約（`kernel/`・`include/`・
   `library/` 編集禁止、変更は `target/`・`syssvc/`・新規ファイルに限定）に従う。
   本リポジトリの作業は **FSP 側ファイル（`asp3/target/`・`asp3/asp3_fsp.cmake`・各ボード）** に閉じる。
2. **カーネル内で動的メモリ確保を使わない**（`malloc`/`new` 等禁止。静的生成のみ）。

## 2. 取得・ビルド・実機確認

```bash
git clone --recurse-submodules https://github.com/exshonda/asp3_fsp.git
# 既存clone: git submodule update --init --recursive
```

- 各 `*/sample/` を CMake・VS Code・Smart Configurator のアプリケーションルートとして扱う。
- **RASC で構成生成が必要**（GUI 依存のため CI ビルドは行わない）。手順は README.md を参照。
- 実機検証は **EK-RA6M5／EK-RA8M2**。シリアル出力 `TOPPERS/ASP3 Kernel …` →
  `task1 is running (NNN).` の周期出力で基本動作を確認する。

## 検証の鉄則

- コードを変更したら **必ずビルドを通してから報告**。「動くはず」で報告しない。
- 実機確認はシリアル出力を根拠とする。
- asp3_core 側に変更が要る場合は別リポジトリで行い、push 権限が無ければ差分を提示して依頼。

## 参考

| 参照 | 用途 |
|---|---|
| `asp3/asp3_core/docs/dev/fsp-integration.md` | FSP 統合の正本（経緯・設計） |
| `asp3/asp3_core/AGENTS.md` | カーネル本体の規約 |
| `.claude/skills/porting-asp3-to-renesas-ra/` | RA への移植ガイド skill |
