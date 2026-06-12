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

### 1. ディレクトリ構成（2026-06-11 submodule 移行後）

カーネル本体は **asp3_core サブモジュール**から供給し、本リポジトリは FSP 固有部だけを持つ
（旧・カーネル同梱 fork 構成は廃止。経緯は `asp3/asp3_core/docs/dev/fsp-integration.md`）。

```
asp3_fsp/
├── asp3/
│   ├── asp3_core/                 # submodule（純カーネル: kernel/ cfg/ syssvc/ include/
│   │   │                          #   library/ ＋ arch/arm_m_gcc/common/ 等の共通arch）
│   │   └── （※直接編集禁止。変更は asp3_core リポジトリ側で）
│   ├── asp3_fsp.cmake             # FSP 協調ヘルパ（ASP3_TARGET_DIR/ASP3_CORE_DIR 解決）
│   ├── arch/arm_m_gcc/
│   │   └── ra<X>m<Y>_fsp/         # MCU 系列固有のアーキ依存部（チップ依存部・本リポジトリ）
│   └── target/
│       └── ek_ra<X>m<Y>/          # ボード固有ターゲット依存部（本リポジトリ）
└── ek_ra<X>m<Y>/sample/           # FSP プロジェクト
    ├── CMakeLists.txt
    ├── Config.cmake               # RASC パス等
    ├── configuration.xml          # RASC 設定（パック参照を含む）
    ├── cmake/
    │   ├── GeneratedCfg.cmake     # RASC 自動生成（編集不可）
    │   ├── GeneratedSrc.cmake     # RASC 自動生成（編集不可）
    │   ├── asp3_sections.lld      # ASP3 固有セクション配置（.vector 等）
    │   └── llvm.cmake             # LLVM ツールチェーン定義
    ├── ra/                        # FSP ソース（RASC 生成）.gitignore対象
    └── ra_gen/                    # FSP 生成ファイル（RASC 生成）.gitignore対象
```

新規ボード追加では `asp3/target/ek_ra<新>`・（必要なら）`asp3/arch/arm_m_gcc/ra<新>_fsp`・
`ek_ra<新>/sample` をセットで用意する。

**CMake 統合モデル**（sample/CMakeLists.txt の骨格）:

```cmake
set(ASP3_TARGET ek_ra<XY>)
include(../../asp3/asp3_fsp.cmake)        # ASP3_TARGET_DIR / ASP3_CORE_DIR / ASP3_ROOT_DIR を解決
set(ASP3_APPLDIR  ${ASP3_CORE_DIR}/sample)
set(ASP3_APPLNAME sample1)
set(ASP3_LIBRARY_ONLY ON CACHE BOOL "" FORCE)
add_subdirectory(${ASP3_CORE_DIR} asp3)   # asp3 lib／cfg 3パス／ヘルパ関数のみ公開
target_link_libraries(${CMAKE_PROJECT_NAME}.elf asp3)
asp3_add_syssvc(${CMAKE_PROJECT_NAME}.elf)  # syslog/banner/serial/logtask＋library
# cfg1_out（静的API値抽出ELF）は FSP 生成ヘッダに依存 → RASC 生成後に
add_dependencies(cfg1_out generate_content_${CMAKE_PROJECT_NAME})
```

**パス解決規約**（asp3_core PORTING_GUIDE「外部ターゲット」）:
- 共通arch（`arch/arm_m_gcc/common`）・カーネル＝`${ASP3_ROOT_DIR}`（submodule 側）
- チップ依存部（`ra*_fsp`）・ターゲット依存部＝`${CMAKE_CURRENT_LIST_DIR}` 相対（本リポジトリ側）

### 1.5 asp3_core サブモジュールとの境界（重要）

- **`asp3/asp3_core/` 配下は直接編集しない**。カーネル・共通archの修正が必要なら
  asp3_core リポジトリ側で行う（同リポジトリの AGENTS.md の禁則＝`kernel/`等は編集禁止・
  動的メモリ確保禁止に従う）。
- 旧 fork が共通archに持っていた M85 対応は **asp3_core main に取込済み**：
  CPACR/FPCCR の bare マクロ廃止（CMSIS 構造体メンバと衝突）／`exc_return_const` の
  `.balign 4`（M85 UNALIGNED fault）／`EXC_RETURN_PREFIX` の `#ifndef` ガード。
- **未取込**：MVE(Helium) の VPR レジスタ保存/復帰（`#ifdef __ARM_FEATURE_MVE`）。
  Helium を使うアプリを M85 で動かす場合は asp3_core 側への取込みが必要
  （recipe はリポジトリ直下 `asp3_change.md` Step5 4a–c）。

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

**Linux（検証済み：ATfE 21.1.1＋RASC 6.2.0・EK-RA6M5/RA8M2 ビルド確認）**：
RASC 同梱の `cmake/llvm.cmake` をツールチェーンファイルとして渡すのが簡単。

```bash
export ARM_TOOLCHAIN_PATH=~/.renesas/platform/arm-llvm/<ver>/ATfE-<ver>-Linux-x86_64/bin
export RASC_EXE_PATH=~/.renesas/platform/sc/ra/fsp_<ver>/eclipse/rasc
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/cmake/llvm.cmake \
  -DARM_TOOLCHAIN_PATH=$ARM_TOOLCHAIN_PATH \
  -DRASC_EXE_PATH=$RASC_EXE_PATH \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

- `Config.cmake` の既定は Windows 前提（`rasc.exe`/`python.exe`）のため、Linux では
  `-DRASC_EXE_PATH=` の明示が必須。
- RASC はヘッドレスで `--generate` 可能（終了時に GUI エラー表示で GTK が落ちることが
  あるが生成物には無害）。

**Windows（ツールチェーンを直接指定する場合）**：

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
- **RASC の FSP バージョンは `configuration.xml` の `#FSPVersion#` と一致が必須**。
  不一致だと `--generate` が「Failed to locate component … in any software packs」で失敗
  （対処は [checklists/fsp-update.md](checklists/fsp-update.md)＝XML側を更新、または
  該当バージョンのパックを導入）。

ツールチェーン定義の雛形: [snippets/llvm-toolchain.cmake](snippets/llvm-toolchain.cmake)。

---

## 5. 動作確認

最小チェック:
1. `ek_ra*/sample/build*/asp3_fsp.elf`（or `.hex`）をフラッシュ（J-Link 等）
   （成果物名は `project()` 名＝`asp3_fsp`。ビルドディレクトリは拡張なら `build/Debug`、
   CLI なら `-B` で指定したもの）
2. シリアル（115200 8N1）で `task1 is running` が出力されればカーネル動作 OK

サンプルアプリ（`asp3/asp3_core/sample/sample1.c`＝submodule 同梱）は周期ハンドラ + タスクの
簡易構成。ターゲット依存部が動けばすぐ出力が見える。

文字化けの切り分け（M85 実機で経験済み）:
- **バナーから全部化ける** → baud/クロック設定（SCICLK・分周）を疑う
- **バナー後に毎ディスパッチで化ける/暴走** → コンテキストスイッチ系。
  `exc_return_const` のアライン（asp3_core で修正済）や FPU/MVE 退避漏れを疑う
- **バナー直後の数バイトだけ化け、その後正常** → 低レベル putc → `R_SCI_*_UART_Open`
  切替時の送信中バイト取りこぼし（TEND 待ち不足）を疑う（SCI_B で既知・未解決）

---

## RASC コマンドライン仕様

`rasc.exe --generate` の使い方と必須/任意オプションは [reference/rasc-cli-options.md](reference/rasc-cli-options.md) を参照。

## MCU 系列ごとの UART API 差異

FSP の UART ドライバは MCU 系列で異なる（`r_sci_uart` vs `r_sci_b_uart` 等）。詳細は [reference/fsp-uart-api.md](reference/fsp-uart-api.md)。

---

## 参考リポジトリ・ドキュメント

- 本リポジトリ: `asp3_fsp`（EK-RA6M5、EK-RA8M2 対応済み）
- 純カーネル: [asp3_core](https://github.com/exshonda/asp3_core)（submodule・public）。
  規約は `asp3/asp3_core/AGENTS.md`、外部ターゲット規約は同 `docs/porting/PORTING_GUIDE.md`、
  FSP統合の経緯・検証記録は同 `docs/dev/fsp-integration.md`
- 旧 fork 時代の移植記録: リポジトリ直下 `asp3_change.md`（M85 FPU/MVE 対応の recipe）
- 動作確認済み環境:
  - Windows: ARM LLVM 18.1.3、FSP 6.4.0、RASC（sc_v2025-12）※旧 fork 構成時代
  - Linux: ATfE clang 21.1.1、FSP/RASC 6.2.0（`configuration.xml` と一致させること）
    ＝submodule 構成で EK-RA6M5/RA8M2 ビルド・EK-RA6M5 実機確認済み
