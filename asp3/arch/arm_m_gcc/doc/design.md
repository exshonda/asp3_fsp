# arm_m_gcc 設計メモ

ARM Cortex-M シリーズ向け ASP3 アーキテクチャ依存部の設計方針。

## ディレクトリ構成

- `common/` : Cortex-M 共通コード（M0/M0+/M3/M4/M7/M23/M33/M85 を `__TARGET_ARCH_THUMB` で分岐）
- `ra6m5_fsp/` : Renesas RA6M5 (Cortex-M33) チップ依存部
- `ra8m2_fsp/` : Renesas RA8M2 (Cortex-M85) チップ依存部
- `doc/` : 設計ドキュメント

## アーキテクチャ分岐の方針

C/アセンブラコード内では `__TARGET_ARCH_THUMB` の値で分岐する。

| アーキテクチャ | コア | `__TARGET_ARCH_THUMB` |
|----------------|------|----------------------|
| ARMv6-M | M0/M0+/M23 | 3 |
| ARMv7-M | M3/M4/M7 | 4 |
| ARMv8-M | M33 | 5 |
| ARMv8.1-M | M85 | 5 以上 |

`TOPPERS_CORTEX_M33` 等のチップ系マクロは `arch.cmake` で識別目的に定義するが、現状コード内では参照していない。

## FPU コンテキスト

`TOPPERS_FPU_CONTEXT` 定義時、コンテキストスイッチで callee-saved の `s16-s31` を保存する。

- M33 (FPv4-SP, single precision): `s16-s31` = 16本
- M85 (FPv5-D16, double precision): `s16-s31` = `d8-d15` で同一範囲
- AAPCS の callee-saved 規約により、M33/M85 で同じコードが流用可能

## TrustZone / PSPLIM

- ARMv8-M / ARMv8.1-M で共通の機構
- `TOPPERS_ENABLE_TRUSTZONE` 定義時に有効化
- `core_support.S` で `psplim` を `__TARGET_ARCH_THUMB >= 5` 時に設定

## Cortex-M85 (RA8M2) 対応方針

### `common/` への変更

不要。既存コードは `__TARGET_ARCH_THUMB` の条件分岐で M85 にも対応する。

### `ra8m2_fsp/` の新規作成

`ra6m5_fsp/` をベースにコピーし、以下を変更：

| ファイル | 変更内容 |
|---------|----------|
| `arch.cmake` | `TOPPERS_CORTEX_M33` → `TOPPERS_CORTEX_M85`<br>include path: `ra6m5_fsp` → `ra8m2_fsp`<br>`ARCH_SERIAL` パスを `ra8m2_fsp` 配下に変更 |
| `chip_stddef.h` | システム略称 `TOPPERS_RA6M5_FSP` → `TOPPERS_RA8M2_FSP` |
| `chip_kernel.h`, `chip_kernel_impl.h` | コメントのみ変更 |
| `chip_rename.def`, `chip_rename.h`, `chip_unrename.h` | コメントのみ変更（リネームシンボルは共通） |
| `chip_sil.h`, `chip_syssvc.h` | コメントのみ変更 |
| `chip_cfg1_out.h` | 必要に応じて再生成 |
| `MANIFEST` | 識別子更新 |

### キャッシュ

M85 の I/D-Cache 初期化は FSP の `SystemInit` / `bsp_init` に委ねる。ASP3 側ではキャッシュ操作 API は提供しない。

### ツールチェイン

`-mcpu=cortex-m85 -mfpu=fpv5-d16 -mfloat-abi=hard` は Smart Configurator が `configuration.xml` から CMake 生成ファイルに反映する。ASP3 側での明示的な指定は不要。

## ToDo

- Helium (MVE) 対応：アプリケーションで MVE を使用する場合のコンテキスト保存（`q0-q7` のうち callee-saved 範囲）対応は後続ステップで検討。
