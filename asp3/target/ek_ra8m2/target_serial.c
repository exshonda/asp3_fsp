/*
 * シリアルインタフェースドライバのターゲット依存部（非TECS版専用）
 *
 * $Id: target_serial.h 289 2021-08-05 14:44:10Z ertl-komori $
 */

#include <stdint.h>
#include "r_sci_b_uart.h"
#include "r_uart_api.h"
#include "kernel_impl.h"
#include "t_stddef.h"
#include <sil.h>
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
    uint8_t snd_byte;               /* 送信中バイト（FSP TXI ISR 用の永続バッファ） */
    uint8_t rcv_buf[256];           /* 受信バッファ */
    uint32_t rcv_wpos;              /* 受信バッファ書き込み位置 */
    uint32_t rcv_rpos;              /* 受信バッファ読み込み位置 */
};

/*
 *  SIOポート管理ブロックのエリア
 */
static SIOPCB siopcb_table[TNUM_PORT] = {
    {0, false, &g_uart0, false, false, 0, {0}, 0, 0},
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
    for (uint_t i = 0; i < TNUM_PORT; i++)
    {
        siopcb_table[i].exinf = exinf;
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

    /*
     * target_initialize() で g_uart0 は既に R_SCI_B_UART_Open 済みだが，ここで
     * 再 Open する．BSP_CFG_PARAM_CHECKING_ENABLE=0 のため FSP の R_SCI_B_UART_Open
     * は ALREADY_OPEN を返さず全レジスタを再初期化し，TXI/TEI 割込みを NVIC で
     * 有効化する（この再初期化を省くと割り込み駆動送信が確立せず，先頭メッセージ
     * 以降が一切出力されないことを実機で確認済み）．
     */
    status = R_SCI_B_UART_Open(p_siopcb->handle->p_ctrl, p_siopcb->handle->p_cfg);
    if (status != FSP_SUCCESS) {
        return NULL;
    }

    /*
     * 再 Open の再初期化過程（CCR0=IDSEL で TE を一旦落とす）で TXD に短い LOW
     * グリッチが生じ，受信側 UART が同期を失う．TXD を idle(High) のまま十分な
     * 時間保持して受信側を再同期させてから実データ送信に入ることで，先頭メッセージ
     * の文字化けを最小化する（このルーチンは起動時に一度だけ実行される）．
     */
    sil_dly_nse(10000000U);

    p_siopcb->is_opend = true;

    return p_siopcb;
}

/*
 * SIOポートのクローズ
 */
void sio_cls_por(SIOPCB *p_siopcb)
{
    R_SCI_B_UART_Close(p_siopcb->handle->p_ctrl);
    p_siopcb->is_opend = false;
}


/*
 * SIOポートへの文字送信
 */
bool_t sio_snd_chr(SIOPCB *p_siopcb, char ch)
{
    sci_b_uart_instance_ctrl_t * p_ctrl = (sci_b_uart_instance_ctrl_t *) p_siopcb->handle->p_ctrl;

    /* TDR が空いていなければ受け付けない．
     * （serial 層はこの false を受けて文字をバッファし，SIO_RDY_SND→sio_irdy_snd
     *   で再送する） */
    if (p_ctrl->p_reg->CSR_b.TDRE == 0) {
        return false;
    }

    /*
     * R_SCI_B_UART_Write は送信のたびに «TE クリア → TE|TIE 同時セット» と TE を
     * トグルする．冷えた最初の送信ではこの TE セットが «TE セット時の 1 フレーム
     * 遅延»（TXD の短い LOW グリッチ）を生じ，先頭バイトに 0xFE 相当の偽フレームが
     * 混入する（実機で確認）．これを避けるため Write は使わず，TE は Open 時の 1 の
     * まま一切トグルしない（RA6M5 の R_SCI_UART_Write と同じ思想）．
     *
     * 具体的には，グリッチのないポーリング送信(target_fput_log_byte)と同様に TDR へ
     * 直接 1 バイト書き込んで送信し，TIE を許可する．TIE のセットだけでは TDRE が
     * 既に 1 のとき TXI のエッジが立たず割込みが起きないため，必ず TDR 書き込みで
     * 送信を起動する点が重要．次バイトは TDR が空く際の TXI(TX_DATA_EMPTY) を契機に
     * 供給する．tx_src_bytes は 0 のままにし，FSP の sci_b_uart_txi_isr には TDR を
     * 書かせず «完了パス»（TEIE セット＋TX_DATA_EMPTY コールバック）だけを通す．
     * その TX_DATA_EMPTY コールバック(target_uart_handler)で TEIE をクリアして TEI を
     * 抑止し，TE が落ちない（=トグルしない）ようにしている．
     */
    p_ctrl->tx_src_bytes  = 0U;
    p_ctrl->p_reg->TDR_BY = (uint8_t)ch;
    p_ctrl->p_reg->CCR0  |= (uint32_t)R_SCI_B0_CCR0_TIE_Msk;
    return true;
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
        /* TIE/TEIE は R_SCI_B_UART_Write が CCR0 で管理するため NVIC は触らない */
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
        /* TIE は FSP の R_SCI_B_UART_Write が CCR0 で管理するため NVIC は触らない */
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
static void target_fput_log_byte(sci_b_uart_instance_ctrl_t *p_ctrl, uint8_t b)
{
    uint32_t reg;
    /* Keep TE=1 (set by R_SCI_B_UART_Open). Disable interrupt enables to prevent
     * ISR interference, wait for TDR empty, write byte, then wait for transmission. */
    reg = p_ctrl->p_reg->CCR0;
    p_ctrl->p_reg->CCR0 &= (uint32_t) ~(R_SCI_B0_CCR0_TIE_Msk | R_SCI_B0_CCR0_TEIE_Msk);
    do { } while (p_ctrl->p_reg->CSR_b.TDRE == 0);
    p_ctrl->p_reg->TDR_BY = b;
    do { } while (p_ctrl->p_reg->CSR_b.TEND == 0);
    p_ctrl->p_reg->CCR0 = reg;
}

void target_fput_log(char c)
{
    SIOPCB *p_siopcb = get_siopcb(FPUT_PORTID);
    sci_b_uart_instance_ctrl_t *p_ctrl = (sci_b_uart_instance_ctrl_t *)p_siopcb->handle->p_ctrl;

    if (c == '\n') {
        target_fput_log_byte(p_ctrl, '\r');
    }
    target_fput_log_byte(p_ctrl, (uint8_t)c);
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

    if (p_args->event == UART_EVENT_TX_DATA_EMPTY) {
        /*
         * TDR が空き次のバイトを受け付け可能になった通知．ここで «次バイト供給» を
         * 行うことで，バイトを連続供給して TE を一切トグルしない（=先頭グリッチを
         * 生じない）方式とする．
         * FSP の sci_b_uart_txi_isr は tx_src_bytes が 0 になると TEIE を立てる．
         * これを放置すると sci_b_uart_tei_isr が TE をクリアしてしまうため，ここで
         * TEIE をクリアして TEI（=TE クリア）を抑止し，TE を Open 時の 1 のまま維持する．
         */
        sci_b_uart_instance_ctrl_t *p_ctrl =
            (sci_b_uart_instance_ctrl_t *)p_siopcb->handle->p_ctrl;
        p_ctrl->p_reg->CCR0 &= (uint32_t)~R_SCI_B0_CCR0_TEIE_Msk;
        target_uart_txi(p_siopcb);
    } else if (p_args->event == UART_EVENT_TX_COMPLETE) {
        /* TEIE を常にクリアしているため通常は発生しない（保険として無処理）． */
    } else if (p_args->event == UART_EVENT_RX_CHAR) {
        target_uart_rxi(p_siopcb, p_args->data);
    } else if (p_args->event == UART_EVENT_ERR_PARITY ||
               p_args->event == UART_EVENT_ERR_FRAMING ||
               p_args->event == UART_EVENT_ERR_OVERFLOW) {
        target_uart_eri(p_siopcb);
    }
}