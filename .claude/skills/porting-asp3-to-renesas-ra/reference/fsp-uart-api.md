# MCU 系列ごとの FSP UART API 差異

FSP の UART ドライバは MCU 系列によって名称が異なるため、`target_serial.c` を新ボードに移植する際は対応する API を呼ぶ必要がある。

## ドライバ系列の対応表

| 系列名 | 対象 MCU | ヘッダ | API プレフィクス | コントロール型 | 設定型 |
|-------|----------|--------|-----------------|---------------|--------|
| `r_sci_uart` | RA2、RA4、RA6 系（旧） | `r_sci_uart.h` | `R_SCI_UART_` | `sci_uart_instance_ctrl_t` | `uart_cfg_t` |
| `r_sci_b_uart` | RA6T2、RA8 系（新） | `r_sci_b_uart.h` | `R_SCI_B_UART_` | `sci_b_uart_instance_ctrl_t` | `uart_cfg_t` |
| `r_uarta` | RA0 系（一部） | `r_uarta.h` | `R_UARTA_` | `uarta_instance_ctrl_t` | `uart_cfg_t` |

`_b` の有無で実装が分かれているため、ヘッダ・型・関数名すべてが変わる。

## どちらを使うか判定する方法

1. RASC GUI でボードを開き、Stacks タブで UART を見ると使用ドライバが分かる
2. または `ra/fsp/inc/api/r_uart_api.h` をインクルードしている `r_sci_*.h` のいずれかを `#include` する
3. `configuration.xml` 内の `<module id="module.driver.uart_on_<name>">` の `<name>` 部分でも判別できる

## 移植時の置換例（RA6M5 → RA8M2 想定）

`target_serial.c` で:

```c
// RA6M5
#include "r_sci_uart.h"
sci_uart_instance_ctrl_t *p_ctrl = (sci_uart_instance_ctrl_t *)p_siopcb->handle->p_ctrl;
R_SCI_UART_Open(...);
R_SCI_UART_Write(...);
R_SCI_UART_Close(...);
```

```c
// RA8M2
#include "r_sci_b_uart.h"
sci_b_uart_instance_ctrl_t *p_ctrl = (sci_b_uart_instance_ctrl_t *)p_siopcb->handle->p_ctrl;
R_SCI_B_UART_Open(...);
R_SCI_B_UART_Write(...);
R_SCI_B_UART_Close(...);
```

## レジスタアクセス

`p_ctrl->p_reg->SSR_b.TDRE` のようなレジスタフィールドアクセスもドライバごとに微妙に異なる場合があるので、生成された `hal_data.c` や FSP のサンプルを参照する。

## RX 割込み・コールバック

コールバック関数のシグネチャは `uart_callback_args_t *p_args` で共通だが、`p_args->event` の enum 値（`UART_EVENT_*`）は共通。各 IRQ 種別ごとの処理（`UART_EVENT_TX_COMPLETE`、`UART_EVENT_RX_CHAR`、`UART_EVENT_ERR_*`）は基本同じコードで動く。

## タイマー（`r_gpt` 等）も同様

UART 以外にも `r_gpt`（GPT タイマ）など `_b` 付き版がある周辺がある。`target_timer.c` を移植するときも同様に確認する。
