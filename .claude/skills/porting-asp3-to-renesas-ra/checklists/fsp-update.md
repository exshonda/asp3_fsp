# FSP バージョン更新チェックリスト

既存の EK-RA ボードを新しい FSP バージョン（例: 6.1.0 → 6.4.0）に更新する手順。

## 事前確認

- [ ] 新 FSP バージョンの RASC（Smart Configurator）がインストール済み
  - 例: `C:/Renesas/RA/sc_v2025-12_fsp_v6.4.0/`
- [ ] 利用可能なパックを確認
  - `<RASC>/internal/projectgen/ra/packs/` 配下に必要な `.pack` ファイルがあること
  - 必要なパック例（RA6M5 ボードの場合）:
    - `Renesas.RA.<新>.pack`
    - `Renesas.RA_common.<新>.pack`
    - `Renesas.RA_board_ra6m5_ek.<新>.pack`
    - `Renesas.RA_mcu_ra6m5.<新>.pack`
    - `Arm.CMSIS6.6.1.0+fsp.<新>.pack`（CMSIS バージョンは別管理）

## configuration.xml のバージョン参照を全置換

旧バージョンのまま `rasc.exe --generate` を実行すると **「Error extracting CMSIS components」** で失敗するため、`configuration.xml` 内の全パック参照を新バージョンに書き換える。

- [ ] `<option key="#FSPVersion#" value="..."/>` を更新
- [ ] 各 `<component ... version="..." />` の `version` 属性を更新
- [ ] 各 `<originalPack>...</originalPack>` の `.pack` ファイル名を更新
- [ ] CMSIS の `+fsp.<旧>` 接尾辞を `+fsp.<新>` に更新

一括置換例（FSP 6.1.0 → 6.4.0、RA6M5）:

```bash
sed -i 's/fsp\.6\.1\.0/fsp.6.4.0/g;
        s/version="6\.1\.0"/version="6.4.0"/g;
        s/value="6\.1\.0"/value="6.4.0"/g;
        s/RA\.6\.1\.0\.pack/RA.6.4.0.pack/g;
        s/RA_common\.6\.1\.0\.pack/RA_common.6.4.0.pack/g;
        s/RA_board_ra6m5_ek\.6\.1\.0\.pack/RA_board_ra6m5_ek.6.4.0.pack/g;
        s/RA_mcu_ra6m5\.6\.1\.0\.pack/RA_mcu_ra6m5.6.4.0.pack/g' \
   configuration.xml
```

ボードや MCU が異なる場合は、対応するパック名にあわせて sed パターンを追加。

- [ ] 置換漏れがないか確認: `grep "<旧バージョン>" configuration.xml` で何も出ないこと

## Config.cmake の更新

- [ ] `RASC_EXE_PATH` のデフォルトパスを新 RASC のものに更新

```cmake
set(RASC_EXE_PATH "C:/Renesas/RA/sc_v2025-12_fsp_v6.4.0/eclipse/rasc.exe")
```

## RASC で再生成

- [ ] `ra/` と `ra_gen/` を削除してから再生成（古いファイルが残らないように）

```bash
rm -rf ek_ra<XY>/sample/ra ek_ra<XY>/sample/ra_gen

"<RASC>/eclipse/rasc.exe" \
  -nosplash --launcher.suppressErrors \
  --generate --devicefamily ra --compiler LLVMARM \
  --toolchainversion 18.1.3 --buildconfiguration Debug \
  ek_ra<XY>/sample/configuration.xml
```

`--buildconfiguration Debug` は **必須**（RASC v6.4.0 で省略すると失敗するケースあり）。

## バージョン確認

- [ ] `ek_ra<XY>/sample/ra/fsp/inc/fsp_version.h` で `FSP_VERSION_*` が新バージョンになっていることを確認

```c
#define FSP_VERSION_MAJOR (6U)
#define FSP_VERSION_MINOR (4U)
#define FSP_VERSION_PATCH (0U)
```

## ビルド・動作確認

- [ ] `build/Debug` を削除して cmake reconfigure
- [ ] 警告ゼロ・エラーゼロでビルド完了
- [ ] 実機で `task1 is running (NNN).` を確認

## FSP API の breaking change 対応

FSP メジャー/マイナーバージョン間で API が変わることがある。ビルドエラーになったら以下を確認:

- [ ] `target_serial.c` の `R_SCI_UART_*` 等 API シグネチャが変わっていないか
- [ ] `hal_data.h` で生成される `g_uart0_ctrl` 等の型名が変わっていないか
- [ ] `target_kernel_impl.c` の `R_BSP_*` 初期化 API が残っているか

破壊的変更があった場合は FSP のリリースノートを参照して呼び出し側を修正する。

## バージョン管理

- [ ] `git status` で `ra/`、`ra_gen/` が `.gitignore` 対象であることを確認（追跡される変更は `configuration.xml`、`bsp_linker_info.h`、`.secure_*` 等のみ）
- [ ] バックアップとして `configuration.xml.bak` を作っておくと差分確認が容易
