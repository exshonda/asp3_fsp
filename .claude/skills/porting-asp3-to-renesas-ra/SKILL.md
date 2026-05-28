---
name: porting-asp3-to-renesas-ra
description: TOPPERS/ASP3 を Renesas RA シリーズの新しいボード（EK-RA*）に移植する手順と、FSP + RASC + LLVM ツールチェーン環境特有の落とし穴をまとめたガイド。新規 RA ボード対応、FSP バージョン更新、ビルド警告ゼロ化、コマンドラインビルドの整備などの作業で参照する。
---

# TOPPERS/ASP3 を Renesas RA ボードに移植する

このリポジトリ（`asp3_fsp`）は TOPPERS/ASP3 を Renesas FSP 環境で動かすための移植層。EK-RA6M5 / EK-RA8M2 を実例として、新規 RA ボード対応や FSP バージョン更新で必要な作業をまとめる。

## このスキルが扱う作業

1. **新規ボード（例: EK-RA4M2）を ASP3 に追加する** → [checklists/new-board.md](checklists/new-board.md)
2. **既存ボードを新しい FSP バージョンに更新する** → [checklists/fsp-update.md](checklists/fsp-update.md)
3. **ビルド警告を除去する** → [reference/warning-fixes.md](reference/warning-fixes.md)
4. **コマンドラインからビルドできるようにする** → 本ファイル §4

## 重要な前提知識（最初に読む）

### 1. ディレクトリ構成

```
asp3_fsp/
├── asp3/                          # ASP3 カーネル本体（共通）
│   ├── arch/arm_m_gcc/
│   │   ├── common/                # ARM Cortex-M 共通アーキ依存部
│   │   └── ra<X>m<Y>_fsp/         # MCU 系列固有のアーキ依存部
│   └── target/
│       └── ek_ra<X>m<Y>/          # ボード固有ターゲット依存部
└── ek_ra<X>m<Y>/sample/           # FSP プロジェクト
    ├── CMakeLists.txt
    ├── Config.cmake               # RASC パス等
    ├── configuration.xml          # RASC 設定（パック参照を含む）
    ├── cmake/
    │   ├── GeneratedCfg.cmake     # RASC 自動生成（編集不可）
    │   ├── GeneratedSrc.cmake     # RASC 自動生成（編集不可）
    │   └── llvm.cmake             # LLVM ツールチェーン定義
    ├── ra/                        # FSP ソース（RASC 生成）.gitignore対象
    └── ra_gen/                    # FSP 生成ファイル（RASC 生成）.gitignore対象
```

新規ボード追加では `asp3/target/ek_ra<新>` と `ek_ra<新>/sample` の 2 箇所をセットで用意する。

### 2. RASC 自動生成ファイルとの付き合い方（最重要）

**RASC が再生成するファイル**（直接編集してはいけない）:
- `cmake/GeneratedCfg.cmake`、`cmake/GeneratedSrc.cmake`
- `ra/`、`ra_gen/`
- `bsp_linker_info.h`、`.secure_rzone`、`.secure_xml` 等

これらは `configuration.xml` 更新時に RASC プレビルドステップで上書きされる。

**設定を永続化する方法**: `CMakeLists.txt` で `include(GeneratedCfg.cmake)` の **後に** オーバーライド。詳細は [snippets/optimization-override.cmake](snippets/optimization-override.cmake)。

### 3. CMakeLists.txt の既知バグ

RASC が生成する CMakeLists.txt テンプレートには `${CMAKE_SIZE}` を使った POST_BUILD コマンドが含まれるが、`CMAKE_SIZE` は未定義のままで `COMMAND sample.elf` に展開される。**Windows でビルドのたびに ELF を「開く」状態になる**ので必ず対処する。修正例: [snippets/cmake-size-fix.cmake](snippets/cmake-size-fix.cmake)。

### 4. アセンブリと共有されるマクロ

`core_kernel_impl.h` の `INT_IPM` マクロはアセンブリからも `#IIPM_LOCK` 形式で参照される。**マクロ本体に `(uint32_t)` などを追加するとアセンブラがパースに失敗する**。警告除去のキャストは C 側の呼び出し箇所に書く。

---

## 4. コマンドラインからの cmake configure

VS Code 拡張機能経由なら裏でツールチェーン情報が補完されるが、コマンドラインで cmake を叩く場合は明示が必要。

```bash
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
  -DCMAKE_SYSTEM_NAME=Generic \
  -DCMAKE_SYSTEM_PROCESSOR=arm \
  -DCMAKE_C_COMPILER="<llvm bin>/clang.exe" \
  -DCMAKE_CXX_COMPILER="<llvm bin>/clang++.exe" \
  -DCMAKE_ASM_COMPILER="<llvm bin>/clang.exe"
```

ポイント:
- `-G Ninja` を指定しないと NMake になり `nmake` 不在で失敗
- `CMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY` がないとリンク段階で失敗（クロスコンパイル必須）
- `RASC_EXE_PATH=echo` を渡せば RASC プレビルドをスキップできる（`ra/` が既にある場合）

ツールチェーン定義の雛形: [snippets/llvm-toolchain.cmake](snippets/llvm-toolchain.cmake)。

---

## 5. 動作確認

最小チェック:
1. `ek_ra*/sample/build/Debug/sample.elf`（or `.hex`）をフラッシュ（J-Link 等）
2. シリアル（115200 8N1）で `task1 is running` が出力されればカーネル動作 OK

サンプルアプリ（`asp3/sample/sample1.c`）は周期ハンドラ + タスク 1 つの簡易構成。ターゲット依存部が動けばすぐ出力が見える。

---

## RASC コマンドライン仕様

`rasc.exe --generate` の使い方と必須/任意オプションは [reference/rasc-cli-options.md](reference/rasc-cli-options.md) を参照。

## MCU 系列ごとの UART API 差異

FSP の UART ドライバは MCU 系列で異なる（`r_sci_uart` vs `r_sci_b_uart` 等）。詳細は [reference/fsp-uart-api.md](reference/fsp-uart-api.md)。

---

## 参考リポジトリ

- 本リポジトリ: `asp3_fsp`（EK-RA6M5、EK-RA8M2 対応済み）
- 動作確認済み環境: ARM LLVM 18.1.3、Renesas FSP 6.4.0、RASC（FSP 同梱版 sc_v2025-12）
