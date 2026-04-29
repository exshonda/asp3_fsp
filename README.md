# ASP3 FSP

TOPPERS/ASP3 と Renesas FSP を組み合わせたサンプルプロジェクトです。

## フォルダ構成

- `asp3/`: 共通の TOPPERS/ASP3 RTOS 本体
- `ek_ra6m5/`: EK-RA6M5 向けのボードフォルダ
- `ek_ra6m5/sample/`: EK-RA6M5 のサンプルアプリケーション

`ek_ra6m5/sample` を CMake，VSCode，Smart Configurator のアプリケーションルートとして扱います。

## Renesas拡張機能のインストール

左領域にある「拡張機能」アイコンを選択し、「Marketplaceで拡張機能を検索する」に「Renesas」と入力して、Renesas拡張機能をインストールします。

![拡張機能のインストール](ek_ra6m5/sample/images/拡張機能のインストール.png)

「QUICK INSTALL」で「Renesas RA」のツールチェインなど一式をインストールします。
このプロジェクトは「Renesas Platform: ARM LLVM Toolchain - 18.1.3」で作成しています。

![ツールチェインのインストール](ek_ra6m5/sample/images/ツールチェインのインストール.png)

## サンプルの開き方

詳細な手順は [ek_ra6m5/sample/README.md](ek_ra6m5/sample/README.md) を参照してください。

VSCode では `ek_ra6m5/sample/asp3_fsp.code-workspace` を開いてください。
