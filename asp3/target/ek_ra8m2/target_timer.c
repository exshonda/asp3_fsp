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

/*
 *  GTCNT(32bit, 10ns/count, 約42.9秒で1周)の上位拡張カウンタ．
 *  オーバーフロー(TIMER_EVENT_CYCLE_END)割込みでインクリメントされるため，
 *  読出し側との競合検出ができるよう volatile とする．
 */
volatile uint32_t upper_counter;

/*
 *  GTCNT の一貫読み
 *
 *  RA8M2 の GPT はカウントクロックに専用 GPTCLK(100MHz) を用いる一方，
 *  レジスタアクセスは別のバスクロックドメインで行われる．非同期クロック
 *  ドメインをまたいで更新中の 32bit カウンタを読むと，まれに桁上がり
 *  伝搬中の不整合値（実際のカウントと大きく異なる値）が読める．これが
 *  時刻の前後ジャンプ（タスク起床の数十秒遅延・task_loop キャリブレー
 *  ションの破壊・スプリアス割込みの洪水）の根本原因となる．
 *  連続した 2 回の読み値が十分近い（=どちらも正常値）ことを確認できる
 *  まで読み直すことで，正しい値のみを使用する．
 */
#define GTCNT_READ_TOLERANCE	200U	/* 連続読みの許容差（2us 相当）*/

static uint32_t
read_gtcnt_consistent(void)
{
	uint32_t prev, cur;

	prev = g_timer0_ctrl.p_reg->GTCNT;
	for (;;) {
		cur = g_timer0_ctrl.p_reg->GTCNT;
		if ((uint32_t)(cur - prev) <= GTCNT_READ_TOLERANCE) {
			return cur;
		}
		prev = cur;
	}
}

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
	uint32_t upper1, upper2, cnt;

	/*
	 *  GTCNT と upper_counter の読出しは非アトミックであり，間に
	 *  オーバーフロー割込み(CYCLE_END, upper_counter++)が入ると
	 *  約42.9秒ずれた時刻を返してしまう（カーネル時刻が前後に飛び，
	 *  タイムイベントの起床が数十秒遅れる間欠不具合の原因になる）．
	 *  読出し前後で upper_counter が変化していないことを確認できる
	 *  まで読み直すことで，一貫した時刻を得る．
	 *  オーバーフロー割込みは NVIC 優先度 0（CPUロックでもマスクされ
	 *  ない）のため，このループが長時間回り続けることはない．
	 */
	do {
		upper1 = upper_counter;
		cnt    = read_gtcnt_consistent();
		upper2 = upper_counter;
	} while (upper1 != upper2);

	/* 1 count = 10 ns より，100で割って us に変換する */
	return((HRTCNT)((((uint64_t)upper1 << 32) + cnt) / 100ULL));
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
	uint32_t cnt_before, cnt_after, delta;

	// ra_gen\hal_data.c: Actual period: 42.94967296 seconds, 1 count = 10 ns
	// 1 count = 10 ns より，hrtcnt[us] を 100倍して 10ns カウント数に変換する．
	// hrtcnt <= HRTCNT_BOUND(40,000,000) なので 100*hrtcnt は 32bit に収まる．
	delta = (uint32_t)(100ULL * (uint64_t)hrtcnt);

	cnt_before = read_gtcnt_consistent();
	R_GPT_CompareMatchSet(&g_timer0_ctrl, (uint32_t)(cnt_before + delta),
						   TIMER_COMPARE_MATCH_A);

	/*
	 *  コンペアマッチ設定が完了するまでに GTCNT が目標値を通過してしまうと，
	 *  一致は次の 32bit ラップ（約42.9秒）まで発生せず，高分解能タイマ割込みが
	 *  その間ずっと起きない（=全タスクが起床せず出力が止まる）．これは特に
	 *  hrtcnt が小さい場合や，割込み禁止区間が長くて割込み処理が遅延した場合に
	 *  起こり得る．ASP3 の target_hrt_set_event の規約どおり，設定時点で既に
	 *  指定時刻が経過していたら割込みを即時要求する．
	 *  設定後に再度カウンタを読み，経過カウント(cnt_after-cnt_before, 32bit
	 *  ラップ込み)が delta 以上なら「通過済み」と判断して手動で割込みを起こす．
	 *  hrtcnt==0(delta==0) の場合も常に即時要求となり正しい．
	 */
	cnt_after = read_gtcnt_consistent();
	if ((uint32_t)(cnt_after - cnt_before) >= delta) {
		target_hrt_raise_event();
	}
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
