/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 * 
 *  Copyright (C) 2000-2003 by Embedded and Real-Time Systems Laboratory
 *                              Toyohashi Univ. of Technology, JAPAN
 *  Copyright (C) 2005-2024 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 * 
 *  上記著作権者は，以下の(1)～(4)の条件を満たす場合に限り，本ソフトウェ
 *  ア（本ソフトウェアを改変したものを含む．以下同じ）を使用・複製・改
 *  変・再配布（以下，利用と呼ぶ）することを無償で許諾する．
 *  (1) 本ソフトウェアをソースコードの形で利用する場合には，上記の著作
 *      権表示，この利用条件および下記の無保証規定が，そのままの形でソー
 *      スコード中に含まれていること．
 *  (2) 本ソフトウェアを，ライブラリ形式など，他のソフトウェア開発に使
 *      用できる形で再配布する場合には，再配布に伴うドキュメント（利用
 *      者マニュアルなど）に，上記の著作権表示，この利用条件および下記
 *      の無保証規定を掲載すること．
 *  (3) 本ソフトウェアを，機器に組み込むなど，他のソフトウェア開発に使
 *      用できない形で再配布する場合には，次のいずれかの条件を満たすこ
 *      と．
 *    (a) 再配布に伴うドキュメント（利用者マニュアルなど）に，上記の著
 *        作権表示，この利用条件および下記の無保証規定を掲載すること．
 *    (b) 再配布の形態を，別に定める方法によって，TOPPERSプロジェクトに
 *        報告すること．
 *  (4) 本ソフトウェアの利用により直接的または間接的に生じるいかなる損
 *      害からも，上記著作権者およびTOPPERSプロジェクトを免責すること．
 *      また，本ソフトウェアのユーザまたはエンドユーザからのいかなる理
 *      由に基づく請求からも，上記著作権者およびTOPPERSプロジェクトを
 *      免責すること．
 * 
 *  本ソフトウェアは，無保証で提供されているものである．上記著作権者お
 *  よびTOPPERSプロジェクトは，本ソフトウェアに関して，特定の使用目的
 *  に対する適合性も含めて，いかなる保証も行わない．また，本ソフトウェ
 *  アの利用により直接的または間接的に生じたいかなる損害に関しても，そ
 *  の責任を負わない．
 * 
 */

/*
 * ターゲット依存モジュール（EK-RA8M2用）
 */
#include "kernel_impl.h"
#include <sil.h>
#include "hal_data.h"

#define SERIAL_PIN_CFG                    ((uint32_t) IOPORT_CFG_DRIVE_HIGH | \
                                           (uint32_t) IOPORT_CFG_PERIPHERAL_PIN | \
                                           (uint32_t) IOPORT_PERIPHERAL_SCI1_3_5_7_9)
#define SERIAL_BAUD_RATE                    (115200U)
#define SERIAL_ERR_X1000                    (4000U)
#define SERIAL_MODULATION                   (0U)

#ifndef TOPPERS_OMIT_TECS
/*
 *  システムログの低レベル出力のための初期化
 *
 */
extern void tPutLogSIOPort_initialize(void);
#endif

/*
 * ターゲット依存部 初期化処理
 */
void
target_initialize(void)
{
    uint32_t status;

    /*
     * コア依存部の初期化
     */
    core_initialize();

    /*
     *  使用するペリフェラルにクロックを供給
     */
#ifndef TOPPERS_OMIT_TECS
    tPutLogSIOPort_initialize();
#endif /* TOPPERS_OMIT_TECS */

    //R_BSP_PinAccessEnable();
    //R_BSP_PinCfg(MIKROBUS_RX_ARDUINO_RX, SERIAL_PIN_CFG);
    //R_BSP_PinCfg(MIKROBUS_TX_ARDUINO_TX, SERIAL_PIN_CFG);
    //R_BSP_PinAccessDisable();

    status = R_SCI_UART_Open(&g_uart0_ctrl, &g_uart0_cfg);
    //status = R_SCI_UART_BaudCalculate(SERIAL_BAUD_RATE, SERIAL_MODULATION, SERIAL_ERR_X1000, g_uart0_cfg_extend.p_baud_setting);
    //status = R_SCI_UART_BaudSet(&g_uart0_ctrl, g_uart0_cfg_extend.p_baud_setting);
}

/*
 * ターゲット依存部 終了処理
 */
void
target_exit(void)
{
    /* チップ依存部の終了処理 */
    core_terminate();
    while(1) ;
}

/*
 *  デフォルトのsoftware_term_hook（weak定義）
 */
__attribute__((weak))
void software_term_hook(void)
{
}

void HardFault_Handler(void) {core_exc_entry();}
void MemManage_Handler(void) {core_exc_entry();}
void BusFault_Handler(void) {core_exc_entry();}
void UsageFault_Handler(void) {core_exc_entry();}
void SecureFault_Handler(void) {core_exc_entry();}
//void SVC_Handler(void) {core_exc_entry();}
void DebugMon_Handler(void) {core_exc_entry();}
//void PendSV_Handler(void) {core_exc_entry();}
//void SysTick_Handler(void) {core_exc_entry();}
