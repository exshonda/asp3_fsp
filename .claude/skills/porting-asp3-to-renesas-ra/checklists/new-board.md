# 新規 RA ボード追加チェックリスト

新しい EK-RA ボード（例: EK-RA4M2）に TOPPERS/ASP3 を追加するときの作業手順。

## 事前準備

- [ ] RASC（Smart Configurator）で対象ボードの BSP/HAL プロジェクトを 1 度生成しておく
- [ ] FSP バージョン（例: 6.4.0）と LLVM ツールチェーンの場所を確認

## ASP3 ターゲット依存部（`asp3/target/ek_ra<XY>/`）

既存ボード（`ek_ra6m5` または `ek_ra8m2`）からコピーして編集。

- [ ] `target.cmake` — MCU 識別マクロ、ASP3 ソース一覧、`include(${CHIPDIR}/arch.cmake)` 行
- [ ] `target_kernel.cfg` / `target_kernel.py`（cfg は asp3_core の Python 版。旧 .trb は廃止）
- [ ] `target_kernel.h` — クロック・割込み番号定義
- [ ] `target_kernel_impl.c/h` — `target_initialize()` 内で FSP 初期化 API を呼ぶ
- [ ] `target_serial.c/h` — 使用 UART ドライバを MCU に合わせる（`r_sci_uart` vs `r_sci_b_uart`）
- [ ] `target_timer.c/h` / `target_timer.cfg`
- [ ] `target_sil.h`, `target_syssvc.h`, `target_stddef.h`, `target_test.h`
- [ ] `target_rename.def`, `target_rename.h`, `target_unrename.h`

`target.cmake` の最小構成例（**外部ターゲット規約**。実例は `asp3/target/ek_ra6m5/target.cmake`）:

```cmake
# パス解決：共通arch＝asp3_core(submodule)、チップ/ターゲット＝本リポジトリ
set(ARCHDIR ${ASP3_ROOT_DIR}/arch/arm_m_gcc)
get_filename_component(CHIPDIR ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm_m_gcc/ra<XY>_fsp ABSOLUTE)
set(TARGETDIR ${CMAKE_CURRENT_LIST_DIR})

list(APPEND ASP3_CFG_FILES            ${TARGETDIR}/target_kernel.cfg)
list(APPEND ASP3_KERNEL_CFG_TRB_FILES ${TARGETDIR}/target_kernel.py)
list(APPEND ASP3_CHECK_TRB_FILES      ${TARGETDIR}/target_check.py)

list(APPEND ASP3_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/ra/...        # FSP/CMSIS の include 群（既存ボード参照）
    ${TARGETDIR}
)

list(APPEND ASP3_COMPILE_DEFS
    RA<XY>                              # MCU 識別マクロ
    $<$<CONFIG:Debug>:DEBUG>
    USE_TIM_AS_HRT
    # FPU を使うなら以下も
    TOPPERS_FPU_ENABLE
    TOPPERS_FPU_LAZYSTACKING
    TOPPERS_FPU_CONTEXT
)
list(APPEND ASP3_TARGET_C_FILES
    ${TARGETDIR}/target_kernel_impl.c
    ${TARGETDIR}/target_timer.c
    ${TARGETDIR}/target_serial.c
)

# FSP/RASC のフラグ・定義・include を asp3 lib と cfg1_out へ伝播
list(APPEND ASP3_COMPILE_OPTIONS ${RASC_CMAKE_C_FLAGS})
list(APPEND ASP3_COMPILE_DEFS    ${RASC_CMAKE_DEFINITIONS})
list(APPEND ASP3_INCLUDE_DIRS    ${CMAKE_SOURCE_DIR})   # bsp_linker_info.h 等

# cfg1_out（静的API値抽出ELF）の最小リンク：arch＋picolibc crt0 回避
list(APPEND ASP3_LINK_OPTIONS
    --target=arm-none-eabi -mcpu=cortex-m<NN> -mfpu=<FPU> -mfloat-abi=hard -mthumb
    -nostartfiles -nostdlib)
list(APPEND ASP3_LINK_LIBS c)

include(${CHIPDIR}/arch.cmake)
```

## ASP3 アーキテクチャ依存部（`asp3/arch/arm_m_gcc/ra<XY>_fsp/`）

- [ ] 既存の `ra6m5_fsp` または `ra8m2_fsp` からコピー
- [ ] `arch.cmake` — `TOPPERS_CORTEX_M<NN>`、`__TARGET_FPU_*` を MCU に合わせる。
      include は `${CHIPDIR}`／共通archは `${ARCHDIR}/common`／`${ASP3_ROOT_DIR}/arch/gcc` を参照
      （`${PROJECT_SOURCE_DIR}` は使わない＝外部ターゲット規約）
- [ ] 共通archの実体（`core_kernel_impl.c` 等）は asp3_core(submodule) 側。コピー不要・編集禁止

## FSP プロジェクト（`ek_ra<XY>/sample/`）

- [ ] `CMakeLists.txt` を既存ボードからコピー、`ASP3_TARGET` をボード名に変更
      （asp3 取り込みは `include(../../asp3/asp3_fsp.cmake)`＋
      `add_subdirectory(${ASP3_CORE_DIR} asp3)`〔`ASP3_LIBRARY_ONLY=ON`〕＋
      `asp3_add_syssvc()`＋`add_dependencies(cfg1_out generate_content_<proj>)` の骨格を維持）
- [ ] `Config.cmake` で RASC パスを正しい FSP バージョンに設定
- [ ] `configuration.xml` を RASC で生成
- [ ] `cmake/llvm.cmake` を配置（既存ボードからコピー可、内容は MCU 非依存）
- [ ] `.vscode/settings.json` の `device` を新ボードの MCU 名に変更

## CMakeLists.txt の修正（既知バグ対処）

- [ ] `${CMAKE_SIZE}` バグ対処を入れる → [../snippets/cmake-size-fix.cmake](../snippets/cmake-size-fix.cmake)
- [ ] `-Os` → `-O2` オーバーライドを入れる（必要なら） → [../snippets/optimization-override.cmake](../snippets/optimization-override.cmake)

## ビルド・動作確認

- [ ] `cmake -S . -B build/Debug -G Ninja ...` で configure 成功
- [ ] `cmake --build build/Debug` で警告ゼロ・エラーゼロ
- [ ] `asp3_fsp.elf`（`project()` 名）をフラッシュ
- [ ] シリアル出力で `task1 is running (NNN).` を確認

## 既存ボードとの差分が多い箇所

| 項目 | RA6M5 (M33) | RA8M2 (M85) |
|------|-------------|-------------|
| MCU 識別 | `RA6M5` | `RA8M2` |
| Cortex | `TOPPERS_CORTEX_M33` | `TOPPERS_CORTEX_M85` |
| FPU 型 | `__TARGET_FPU_FPV4_SP` | `__TARGET_FPU_FPV5_D16` |
| UART API | `R_SCI_UART_*` | `R_SCI_B_UART_*` |
| arch ディレクトリ | `ra6m5_fsp` | `ra8m2_fsp` |
| MVE (Helium) | なし | あり（VPR 保存・復帰が必要） |

新ボードを追加する際はこの表に新行を追加して MCU の差分を整理する。
