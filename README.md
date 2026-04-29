# ASP3 FSP

TOPPERS/ASP3 と Renesas FSP を組み合わせたサンプルプロジェクトです。

## フォルダ構成

- `asp3/`: 共通の TOPPERS/ASP3 RTOS 本体
- `ek_ra6m5/`: EK-RA6M5 向けのボードフォルダ
- `ek_ra6m5/sample/`: EK-RA6M5 のサンプルアプリケーション

`ek_ra6m5/sample` を CMake，VSCode，Smart Configurator のアプリケーションルートとして扱います。

## サンプルの開き方

VSCode では `ek_ra6m5/sample/asp3_fsp.code-workspace` を開いてください。

Smart Configurator でコードを再生成する場合は `ek_ra6m5/sample/configuration.xml` を使用します。
