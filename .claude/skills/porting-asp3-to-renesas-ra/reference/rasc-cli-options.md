# RASC コマンドライン仕様

RASC = Renesas Smart Configurator。`rasc.exe --generate` で FSP プロジェクトのコード生成ができる。

## 基本形

```bash
"<RASC>/eclipse/rasc.exe" \
  -nosplash --launcher.suppressErrors \
  --generate \
  --devicefamily ra \
  --compiler LLVMARM \
  --toolchainversion <ver> \
  --buildconfiguration <Debug|Release> \
  <path-to-configuration.xml>
```

## オプション詳細

| オプション | 必須/任意 | 値の例 | 説明 |
|-----------|----------|--------|------|
| `-nosplash` | 推奨 | — | スプラッシュ画面を出さない（GUI 起動回避） |
| `--launcher.suppressErrors` | 推奨 | — | ランチャーのエラーポップアップを抑制 |
| `--generate` | **必須** | — | プロジェクトコード生成モード |
| `--devicefamily` | **必須** | `ra` | RA 系列 |
| `--compiler` | **必須** | `LLVMARM` / `GNUARM` | 使用ツールチェーン |
| `--toolchainversion` | **必須** | `18.1.3` | LLVM のバージョン |
| `--buildconfiguration` | **必須**（v6.4.0 以降） | `Debug` / `Release` | ビルド設定。**省略すると失敗するケースあり** |
| `<configuration.xml>` | **必須** | プロジェクトの xml | 最終引数として渡す |

## SmartBundle 生成

別オプション `--gensmartbundleandpartition`（v6.4.0）または `--gensmartbundle`（古い版）。
GeneratedSrc.cmake の POST_BUILD で自動実行される。ビルド時間を削るためにスキップしたい場合は `RASC_EXE_PATH` をラッパースクリプトに置換する。

## エラー対処

### `Error extracting CMSIS components`

`configuration.xml` が古い FSP バージョンのパックを参照しているのに、新 RASC にそのパックがない場合。
→ [../checklists/fsp-update.md](../checklists/fsp-update.md) を参照して全パック参照を新バージョンに更新する。

### `Java was started but returned exit code=1`

RASC が内部 Java で起動するときの一般的な失敗メッセージ。詳細は `<configuration.xml と同じディレクトリ>/rasc_cmd_log.txt` または `<workspace>/.metadata/.log` に出る。

### コマンドが GUI を開いてしまう

`-nosplash --launcher.suppressErrors` を忘れている、または `--generate` のような action オプションを指定していない可能性。

## RASC をスキップしたい場合

`ra/` と `ra_gen/` が既に存在していて再生成が不要なら、cmake configure 時に `-DRASC_EXE_PATH=echo` を渡すと RASC プレビルドが echo に置換されてスキップされる。

```bash
cmake -S . -B build/Debug -G Ninja \
  -DRASC_EXE_PATH=echo \
  ...
```

## RASC インストールディレクトリの構成

```
C:/Renesas/RA/sc_v2025-12_fsp_v6.4.0/
├── eclipse/
│   ├── rasc.exe                                # 本体
│   └── plugins/                                # Eclipse プラグイン
├── internal/
│   └── projectgen/ra/packs/                    # FSP パック群
│       ├── Renesas.RA.6.4.0.pack
│       ├── Renesas.RA_common.6.4.0.pack
│       ├── Renesas.RA_board_<board>.6.4.0.pack
│       ├── Renesas.RA_mcu_<mcu>.6.4.0.pack
│       └── Arm.CMSIS6.6.1.0+fsp.6.4.0.pack
├── fsp_documentation/
└── uninstall/
```

利用可能なボード・MCU を確認するには `packs/` 配下を `ls | grep RA_board_` 等で検索する。
