# TOPPERS/ASP3 の VS Code 向け Renesas 拡張機能対応（EK-RA8M2）

## ターゲット

- [EK-RA8M2](https://www.renesas.com/ja/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ek-ra8m2-evaluation-kit-ra8m2-mcu-group)

## 前提ツール

以下のツールを事前にインストールしてください。

| ツール | 備考 |
|--------|------|
| VS Code | 最新版推奨 |
| Renesas 拡張機能 | VS Code の Marketplace で「Renesas」を検索 |
| ARM LLVM Toolchain 18.1.3 | Renesas 拡張の「QUICK INSTALL」で導入 |
| Renesas RA Smart Configurator (RASC) | 同上 |
| SEGGER J-Link ドライバ | J-Link OB 経由でデバッグするために必要 |
| Git | コードのダウンロードに使用 |
| シリアルターミナル | TeraTerm など（115200 bps） |

> Renesas 拡張と ARM LLVM Toolchain は、VS Code の拡張機能ビューで「Renesas」を検索し「QUICK INSTALL」で「Renesas RA」を選択するとまとめてインストールできます。

## コードのダウンロード

適当な場所に作業フォルダを作成し、VS Code で開きます。`Ctrl + Shift + @` でターミナルを開き、以下のコマンドを実行します。

```powershell
git clone https://github.com/exshonda/asp3_fsp.git .
```

## ワークスペースを開く

左領域のエクスプローラービューで `ek_ra8m2/sample/asp3_fsp.code-workspace` を選択し、「ワークスペースを開く」ボタンをクリックします。ワークスペースが開かれると CMake の構成が自動的に始まります。

構成が終わるとツールチェーンの選択項目が表示されるので、「Renesas Platform: ARM LLVM Toolchain - 18.1.3」を選択します。

> ツールチェーンが表示されない場合は、環境変数 `ARM_LLVM_TOOLCHAIN_PATH` が設定されているか確認してください。VS Code を起動する前に以下のように設定します。
>
> ```powershell
> $env:ARM_LLVM_TOOLCHAIN_PATH = "C:\Users\<username>\.renesas\platform\arm-llvm\18.1.3\LLVM-ET-Arm-18.1.3-Windows-x86_64\bin"
> ```

## Smart Configurator でコードを再生成する

エクスプローラービューで `configuration.xml` を右クリックし、「Open with RA Smart Configurator」を選択します。

Smart Configurator が開いたら「Generate Project Content」ボタンをクリックしてコードを生成します。

## プロジェクトのビルド

メニューから「ターミナル」→「タスクの実行」→「Build Project」を選択します。

以下のエラーが出た場合は、「ターミナル」→「タスクの実行」→「Configure Project」を選択してから再度ビルドしてください。

```
Components have been added to, or removed from the project. Build may fail until the project is refreshed in your IDE.
```

## EK-RA8M2 に接続してデバッグ

### 接続方法

| 接続 | 内容 |
|------|------|
| デバッグ USB | EK-RA8M2 の「USB Debug」ポート（J-Link OB）と PC を USB ケーブルで接続 |
| シリアル | SCI8（P102=RXD8、P103=TXD8）に USB シリアル変換アダプタを接続し、GND も共通接続 |

- デバッガ種別：**SEGGER J-Link ARM**（J-Link OB 内蔵）
- シリアル設定：**115200 bps、8bit、パリティなし、ストップビット 1**

### デバッグ開始

左領域の「実行とデバッグ」アイコンを選択し、「デバッグの開始（▶）」ボタンをクリックします。`main` 関数で停止しますので、「実行（F5）」ボタンで続行します。

### 期待出力

シリアルターミナルに以下のようなバナーとタスクメッセージが表示されます。

```
TOPPERS/ASP3 Kernel ...
...
task1 is running. [1]
task1 is running. [2]
task2 is running. [1]
...
```

## CLI でビルドする場合

VS Code を使わずコマンドラインでビルドすることもできます。

```powershell
# Debug ビルド
cmake -DARM_LLVM_TOOLCHAIN_PATH="C:/Users/<username>/.renesas/platform/arm-llvm/18.1.3/LLVM-ET-Arm-18.1.3-Windows-x86_64/bin" `
      -DCMAKE_TOOLCHAIN_FILE=cmake/llvm.cmake -G Ninja -B build/Debug
cmake --build build/Debug

# Release ビルド
cmake -DARM_LLVM_TOOLCHAIN_PATH="C:/Users/<username>/.renesas/platform/arm-llvm/18.1.3/LLVM-ET-Arm-18.1.3-Windows-x86_64/bin" `
      -DCMAKE_TOOLCHAIN_FILE=cmake/llvm.cmake -DCMAKE_BUILD_TYPE=Release -G Ninja -B build/Release
cmake --build build/Release
```

## トラブルシュート

| エラーメッセージ | 対処 |
|-----------------|------|
| `Toolchain path not defined` | `ARM_LLVM_TOOLCHAIN_PATH` 環境変数が未設定。VS Code 起動前に設定する |
| `Toolchain path does not exists` | `ARM_LLVM_TOOLCHAIN_PATH` に指定したパスが存在しない。インストール先を確認する |
| `Failed to run RASC generate command` | `RASC_EXE_PATH` が正しくない。`Config.cmake` の既定値（`C:/Renesas/...`）またはインストール先を確認する |
| `Components have been added to, or removed from the project...` | タスク「Configure Project」を実行してから再ビルドする |
| ツールチェーン選択肢が出ない | `cmake-kits.json` の `ARM_LLVM_TOOLCHAIN_PATH` キーに絶対パスを直接記入して再試行する |
