/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 * 
 *  Copyright (C) 2016-2020 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 * 
 *  上記著作権者は，以下の(1)〜(4)の条件を満たす場合に限り，本ソフトウェ
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
 *  $Id: target_timer.c 292 2021-10-11 12:27:17Z ertl-komori $
 */

/*
 *		タイマドライバ（TIM用）
 *		 TIM2をフリーランニング，TIM5を割込み通知用に使用する．
 */

#include "kernel.h"
#include "time_event.h"
#include "kernel_impl.h"
#include "target_timer.h"
#include "r_gpt.h"
#include "hal_data.h"
#include <stdbool.h>

uint32_t upper_counter;

/*
 * タイマの起動処理
 */
void
target_hrt_initialize(intptr_t exinf)
{
	(void)exinf;
	R_GPT_Open(&g_timer0_ctrl, &g_timer0_cfg);
	R_GPT_Start(&g_timer0_ctrl);
}

/*
 * タイマの停止処理
 */
void
target_hrt_terminate(intptr_t exinf)
{
	(void)exinf;
	R_GPT_Stop(&g_timer0_ctrl);
	R_GPT_Close(&g_timer0_ctrl);
}

/*
 * 高分解能タイマの現在のカウント値の読出し
 */
HRTCNT
target_hrt_get_current(void)
{
	timer_status_t status;
	R_GPT_StatusGet(&g_timer0_ctrl, &status);
	return((HRTCNT)((upper_counter * 0x100000000ULL + status.counter)/100ULL));  /* 1 count = 10 ns より，100で割る */
}

/*
 * 高分解能タイマへの割込みタイミングの設定
 *
 * 高分解能タイマを，hrtcntで指定した値カウントアップしたら割込みを発
 * 生させるように設定する．
 */
void
target_hrt_set_event(HRTCNT hrtcnt)
{
	timer_status_t status;
	R_GPT_StatusGet(&g_timer0_ctrl, &status);
	// ra_gen\hal_data.c: Actual period: 42.94967296 seconds, 1 count = 10 ns
	// 1 count = 10 ns より，hrtcnt を 100倍して設定する
	R_GPT_CompareMatchSet(&g_timer0_ctrl, status.counter + 100ULL * hrtcnt, TIMER_COMPARE_MATCH_A);
}

/*
 *  高分解能タイマ割込みの要求
 *
 */
void target_hrt_raise_event(void)
{
    gpt_extended_cfg_t * p_extend = (gpt_extended_cfg_t *) g_timer0_ctrl.p_cfg->p_extend;
    raise_int((INTNO)(p_extend->capture_a_irq + 16));
}

/*
 *  タイマ割込みハンドラ
 */
void
target_hrt_handler(timer_callback_args_t * p_args)
{
	switch(p_args->event) {
	case TIMER_EVENT_CYCLE_END:
		upper_counter++;
		break;
	case TIMER_EVENT_COMPARE_A:
		/*
		*  高分解能タイマ割込みを処理する．
		*/
		signal_time();
		break;
	default:
		break;
	}
}

void
target_systick_handler(void)
{
}
