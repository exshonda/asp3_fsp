/*
 * シリアルインタフェースドライバのターゲット依存部（非TECS版専用）
 *
 * $Id: target_serial.h 289 2021-08-05 14:44:10Z ertl-komori $
 */

#include <stdint.h>
#include "r_sci_uart.h"
#include "r_uart_api.h"
#include "kernel_impl.h"
#include "t_stddef.h"
#include "target_serial.h"
#include "target_syssvc.h"
#include "hal_data.h"

struct sio_port_control_block
{
    intptr_t exinf;
    bool_t is_opend;                /* オープン済みフラグ */
    const uart_instance_t *handle;  /* UARTハンドル */
    bool_t rdy_snd;                 /* 送信可能コールバック */
    bool_t rdy_rcv;                 /* 受信通知コールバック */
    uint8_t rcv_buf[256];           /* 受信バッファ */
    uint32_t rcv_wpos;              /* 受信バッファ書き込み位置 */
    uint32_t rcv_rpos;              /* 受信バッファ読み込み位置 */
};

/*
 *  SIOポート管理ブロックのエリア
 */
static SIOPCB siopcb_table[TNUM_PORT] = {
    {0, false, &g_uart0, false, false, {0}, 0, 0},
};

/*
 *  SIOポートIDから管理ブロックを取り出すためのマクロ
 */
#define INDEX_SIOP(siopid)	((uint_t)((siopid) - 1))
#define get_siopcb(siopid)	(&(siopcb_table[INDEX_SIOP(siopid)]))

/*
 * SIOドライバの初期化
 */
void sio_initialize(intptr_t exinf)
{
    SIOPCB	*p_siopcb;
    for (uint_t i = 0; i < TNUM_PORT; i++)
    {
        p_siopcb = &(siopcb_table[i]);
        siopcb_table[i].exinf = exinf;
        ((uart_cfg_t *)siopcb_table[i].handle->p_cfg)->p_context = (void *)p_siopcb;
    }
}

/*
 * SIOドライバの終了処理
 */
void sio_terminate(intptr_t exinf)
{
	(void)exinf;
    uint_t	i;
    SIOPCB	*p_siopcb;

    for (i = 0; i < TNUM_PORT; i++) {
        p_siopcb = &(siopcb_table[i]);
        if (p_siopcb->is_opend) {
            /*
                *  オープンされているSIOポートのクローズ
                */
            sio_cls_por(&(siopcb_table[i]));
        }
    }
}

/*
 * SIOポートのオープン
 */
SIOPCB *sio_opn_por(ID siopid, intptr_t exinf)
{
    uint32_t status;
    SIOPCB *p_siopcb = NULL;
    if (siopid > TNUM_PORT) {
        return p_siopcb;
    }

    p_siopcb = get_siopcb(siopid);
    /* すでにオープンされている場合はNULLを返す */
    if (p_siopcb->is_opend) {
        return NULL;
    }

    p_siopcb->exinf = exinf;
    p_siopcb->rcv_wpos = 0;
    p_siopcb->rcv_rpos = 0;

    status = R_SCI_UART_Open(p_siopcb->handle->p_ctrl, p_siopcb->handle->p_cfg);
    if (status != FSP_SUCCESS) {
        return NULL;
    }
    p_siopcb->is_opend = true;

    return p_siopcb;
}

/*
 * SIOポートのクローズ
 */
void sio_cls_por(SIOPCB *p_siopcb)
{
    R_SCI_UART_Close(p_siopcb->handle->p_ctrl);
    p_siopcb->is_opend = false;
}

/*
 * SIOポートへの文字送信
 */
bool_t sio_snd_chr(SIOPCB *p_siopcb, char ch)
{
    sci_uart_instance_ctrl_t * p_ctrl = (sci_uart_instance_ctrl_t *) p_siopcb->handle->p_ctrl;
    bool_t result = false;
    if (p_ctrl->p_reg->SSR_b.TDRE == 0) {
        return false; // 送信可能でない場合
    }
    if (R_SCI_UART_Write(p_siopcb->handle->p_ctrl, (uint8_t *)&ch, 1) == FSP_SUCCESS) {
        result = true; // 送信成功
    }
    return result;
}

/*
 * SIOポートからの文字受信
 */
int_t sio_rcv_chr(SIOPCB *p_siopcb)
{
    uint8_t ch;
    if (p_siopcb->rcv_wpos != p_siopcb->rcv_rpos) // 受信バッファにデータがある場合
    {
        ch = p_siopcb->rcv_buf[p_siopcb->rcv_rpos];
        p_siopcb->rcv_rpos = (p_siopcb->rcv_rpos + 1) % sizeof(p_siopcb->rcv_buf); // 読み込み位置を更新
        return ch;
    }
    return -1; // 受信失敗
}

/*
 * SIOポートからのコールバックの許可
 */
void sio_ena_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
    switch (cbrtn) {
    case SIO_RDY_SND:
        p_siopcb->rdy_snd = true;
        enable_int((INTNO)(p_siopcb->handle->p_cfg->txi_irq + 16));
        //enable_int((INTNO)(p_siopcb->handle->p_cfg->tei_irq + 16));
        break;
    case SIO_RDY_RCV:
        p_siopcb->rdy_rcv = true;
        enable_int((INTNO)(p_siopcb->handle->p_cfg->rxi_irq + 16));
        break;
    default:
        break;
    }
}

/*
 * SIOポートからのコールバックの禁止
 */
void sio_dis_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
    switch (cbrtn) {
    case SIO_RDY_SND:
        p_siopcb->rdy_snd = false;
        disable_int((INTNO)(p_siopcb->handle->p_cfg->txi_irq + 16));
        //disable_int((INTNO)(p_siopcb->handle->p_cfg->tei_irq + 16));
        break;
    case SIO_RDY_RCV:
        p_siopcb->rdy_rcv = false;
        disable_int((INTNO)(p_siopcb->handle->p_cfg->rxi_irq + 16));
        break;
    default:
        break;
    }
}

/*
 * SIOポートへの文字出力
 */
void target_fput_log(char c)
{
    SIOPCB *p_siopcb = get_siopcb(FPUT_PORTID);
    sci_uart_instance_ctrl_t *p_ctrl = (sci_uart_instance_ctrl_t *)p_siopcb->handle->p_ctrl;

    if (c == '\n') {
        R_SCI_UART_Write(p_ctrl, (uint8_t *)"\r", 1);
        do { } while (p_ctrl->p_reg->SSR_b.TDRE == 0);
    }
    R_SCI_UART_Write(p_ctrl, (uint8_t *)&c, 1);
    do { } while (p_ctrl->p_reg->SSR_b.TDRE == 0);
}

void target_uart_txi(SIOPCB *p_siopcb)
{
    /*
     *  該当するポートが見つからない場合は何もしない
     */
    if (p_siopcb == NULL) {
        return;
    }

    /*
     *  送信可能コールバックルーチンを呼び出す．
     */
    if (p_siopcb->rdy_snd) {
        sio_irdy_snd(p_siopcb->exinf);
    }
}

void target_uart_rxi(SIOPCB *p_siopcb, uint32_t data)
{
    /*
     *  該当するポートが見つからない場合は何もしない
     */
    if (p_siopcb == NULL) {
        return;
    }

    /*
     *  受信データをバッファに格納する．
     */
    p_siopcb->rcv_buf[p_siopcb->rcv_wpos] = (uint8_t)data;
    p_siopcb->rcv_wpos = (p_siopcb->rcv_wpos + 1) % sizeof(p_siopcb->rcv_buf);

    /*
     *  受信割込み開始
     */

    /*
     *  受信通知コールバックルーチンを呼び出す．
     */
    if (p_siopcb->rdy_rcv) {
        sio_irdy_rcv(p_siopcb->exinf);
    }
}

void target_uart_eri(SIOPCB *p_siopcb)
{
    /*
     *  該当するポートが見つからない場合は何もしない
     */
    if (p_siopcb == NULL) {
        return;
    }

    // エラー処理をここに追加することができます。
}

void target_uart_handler(uart_callback_args_t * p_args)
{
    SIOPCB *p_siopcb = &siopcb_table[INDEX_SIOP(FPUT_PORTID)];

    if (p_args->event == UART_EVENT_TX_COMPLETE){
        target_uart_txi(p_siopcb);
    } else if (p_args->event == UART_EVENT_TX_DATA_EMPTY) {
        //target_uart_txi(p_siopcb);
    } else if (p_args->event == UART_EVENT_RX_CHAR) {
        target_uart_rxi(p_siopcb, p_args->data);
    } else if (p_args->event == UART_EVENT_ERR_PARITY ||
               p_args->event == UART_EVENT_ERR_FRAMING ||
               p_args->event == UART_EVENT_ERR_OVERFLOW) {
        target_uart_eri(p_siopcb);
    }
}