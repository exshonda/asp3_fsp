# ASP3/FSP 変更履歴

## Step 5: EK-RA8M2 FPU/MVE (Helium) 有効化

対象ボード: EK-RA8M2 (Cortex-M85, ARMv8.1-M)

### 変更ファイル一覧

---

### 1. `ek_ra8m2/sample/CMakeLists.txt`

**変更内容**: FPU を強制無効化していた `foreach` ブロックを削除

FSP のビルドシステムが `-mfloat-abi=hard` を設定するが、元のコードでは後から
`-mfloat-abi=soft-fp` を上書きしていた。このブロックを削除することで FSP の
設定がそのまま有効になり、FPU が有効化される。

---

### 2. `asp3/target/ek_ra8m2/target.cmake`

**変更内容**: FPU 関連の定義を有効化（コメントアウトを解除）

```cmake
list(APPEND ASP3_COMPILE_DEFS
    RA8M2
    $<$<CONFIG:Debug>:DEBUG>
    USE_TIM_AS_HRT
    TOPPERS_FPU_ENABLE          # 追加
    TOPPERS_FPU_LAZYSTACKING    # 追加
    TOPPERS_FPU_CONTEXT         # 追加
)
```

---

### 3. `asp3/arch/arm_m_gcc/ra8m2_fsp/arch.cmake`

**変更内容**: FPU 型の定義を追加

```cmake
list(APPEND ASP3_COMPILE_DEFS
    TOPPERS_CORTEX_M85
    __TARGET_ARCH_THUMB=5
    __TARGET_FPU_FPV5_D16    # 追加
    TOPPERS_ENABLE_TRUSTZONE
)
```

---

### 4. `asp3/arch/arm_m_gcc/common/core_support.S`

**変更内容**: MVE (Helium) の VPR レジスタ保存/復帰コードと `.balign 4` 修正

#### (a) `do_dispatch` — VPR 保存

`vpush {s16-s31}` の直後に VPR 保存を追加。`do_dispatch` は非例外コンテキスト
スイッチのため、ハードウェアによる VPR 自動保存が行われない。

```asm
vpush {s16-s31}
#ifdef __ARM_FEATURE_MVE
    vmrs  r12, VPR
    push  {r12}
#endif /* __ARM_FEATURE_MVE */
```

#### (b) `dispatcher_1/2` — VPR 復帰

`vpop {s16-s31}` の直前に VPR 復帰を追加。

```asm
    cbz   r3, dispatcher_2
#ifdef __ARM_FEATURE_MVE
    pop   {r12}
    vmsr  VPR, r12
#endif /* __ARM_FEATURE_MVE */
    vpop  {s16-s31}
ALABEL(dispatcher_2)
```

#### (c) `return_to_thread` FP パス — VPR 復帰

`ldmfd r0!, {r4-r11}` の直後に VPR 復帰を追加。

```asm
    ldmfd  r0!, {r4-r11}
#ifdef __ARM_FEATURE_MVE
    ldmfd  r0!, {r12}
    vmsr   VPR, r12
#endif /* __ARM_FEATURE_MVE */
```

#### (d) `.balign 4` 追加 — バグ修正

`exc_return_const: .word EXC_RETURN` の直前に `.balign 4` を追加。

**問題**: `#ifdef TOPPERS_FPU_CONTEXT` ブロック内の `bx lr` は 16-bit Thumb 命令
(2 バイト)。これにより `.word` が 2 バイトアラインに置かれ、直前の
`LDR.W PC, [PC, #6]` (ワードアクセス) が UNALIGNED fault を引き起こす。

```asm
#endif /* TOPPERS_FPU_CONTEXT */

    .balign 4
ALABEL(exc_return_const)
    .word  EXC_RETURN
```

---

## ビルド警告の除去

以下のファイルの警告をすべてゼロにした。

### `asp3/include/sil.h`

`TOPPERS_SIL_REV_ENDIAN_UINT16` マクロは整数昇格により `unsigned int` を返すが、
戻り値/代入先が `uint16_t` であるため警告が発生していた。

```c
// 修正箇所 (4箇所)
return((uint16_t)TOPPERS_SIL_REV_ENDIAN_UINT16(data));
*mem = (uint16_t)TOPPERS_SIL_REV_ENDIAN_UINT16(data);
```

---

### `asp3/arch/arm_m_gcc/common/arm_m.h`

`EXC_RETURN_PREFIX` が CMSIS `core_cm85.h` でも定義されており再定義警告が発生。
`#ifndef` ガードを追加。

```c
#ifndef EXC_RETURN_PREFIX
#define EXC_RETURN_PREFIX       0xff000000
#endif /* EXC_RETURN_PREFIX */
```

---

### `asp3/kernel/wait.h` / `asp3/kernel/wait.c`

`uint_t tstat` を `uint8_t` フィールド `p_runtsk->tstat` に代入する際の精度損失警告。

```c
p_runtsk->tstat = (uint8_t)tstat;
```

---

### `asp3/arch/arm_m_gcc/common/core_kernel_impl.h`

複数の警告カテゴリを修正:

1. **符号変換 (`INT_IPM` 使用箇所)**: マクロ結果 (`int`) を `uint32_t` に代入する
   C 側の呼び出し箇所にキャストを追加。
   アセンブリからも `#IIPM_LOCK` として参照されるためマクロ本体は変更不可。

   ```c
   set_basepri_max((uint32_t)IIPM_LOCK);
   set_basepri((uint32_t)IIPM_LOCK);
   const uint32_t iipm = (uint32_t)INT_IPM(intpri);
   ```

2. **符号変換 (`~` 演算子)**: `int` 定数の `~` 結果が `int` になり `uint32_t &=` で警告。

   ```c
   tmp &= ~(uint32_t)SYSTIC_TICINT;
   tmp &= ~(uint32_t)NVIC_PENDSTSET;
   ```

3. **符号変換 (`1 <<` 演算子)**: 6箇所で `1` → `1U` に変更。

   ```c
   (1U << (tmp & 0x1f))   // disable_int, enable_int, clear_int, raise_int, probe_int
   (1U << (intno & 0x1f)) // check_intno_cfg
   ```

4. **未使用パラメータ**: 空スタブ関数に `(void)param;` を追加。

   ```c
   // define_inh, i_begin_int, i_end_int
   (void)inhno; (void)int_entry;
   (void)intno;
   // check_intno_clear, check_intno_raise
   (void)intno;
   // define_exc
   (void)exc_entry;
   ```

---

### `asp3/arch/arm_m_gcc/common/core_kernel_impl.c`

1. **`~` 演算子符号変換**: `disable_exc` 内の4箇所に `(uint32_t)` キャスト追加。

   ```c
   tmp &= ~(uint32_t)NVIC_SYS_HND_CTRL_MEM;
   // ...他3箇所
   ```

2. **`set_exc_int_priority` への `INT_IPM` 渡し**: キャスト追加 (2箇所)。

   ```c
   set_exc_int_priority(EXCNO_PENDSV, (uint32_t)INT_IPM(-1));
   set_exc_int_priority(intno, (uint32_t)INT_IPM(intpri));
   ```

3. **`~CCR_STKALIGN` 符号変換**:

   ```c
   sil_andw((void *)CCR_BASE, ~(uint32_t)CCR_STKALIGN);
   ```

4. **`~(0xFF <<` 符号変換**:

   ```c
   tmp &= ~(0xFFU << (8U * (excno & 0x03U)));
   tmp |= iipm << (8U * (excno & 0x03U));
   ```

5. **ループ変数の符号比較**: `int i` → `uint_t i`

   ```c
   for (uint_t i = 0; i < tnum_tsk; ++i) {
   ```

---

### `asp3/target/ek_ra8m2/target_serial.c`

1. **構造体初期化子**: `snd_byte` (スカラー) を `{0}` ではなく `0` で初期化、
   `rcv_buf` と `rcv_rpos` を明示的に追加。

   ```c
   // 変更前
   {0, false, &g_uart0, false, false, {0}, 0, 0}
   // 変更後
   {0, false, &g_uart0, false, false, 0, {0}, 0, 0}
   ```

2. **未使用変数 `p_siopcb`**: `sio_initialize` 内の未使用変数と冗長コードを削除。

   ```c
   // 変更前
   SIOPCB *p_siopcb;
   p_siopcb = &(siopcb_table[i]);
   siopcb_table[i].exinf = exinf;
   // 変更後
   siopcb_table[i].exinf = exinf;
   ```

---

### `asp3/target/ek_ra8m2/target_kernel_impl.c`

未使用変数 `status` を削除し、戻り値を `(void)` キャストに変更。

```c
// 変更前
uint32_t status;
status = R_SCI_B_UART_Open(&g_uart0_ctrl, &g_uart0_cfg);
// 変更後
(void)R_SCI_B_UART_Open(&g_uart0_ctrl, &g_uart0_cfg);
```

---

### `asp3/syssvc/syslog.c`, `serial.c`, `logtask.c`

初期化/終了ルーチンの `exinf` パラメータが未使用。先頭に `(void)exinf;` を追加。

対象関数: `syslog_initialize`, `serial_initialize`, `logtask_main`, `logtask_terminate`

---

### `asp3/syssvc/banner.c`

1. 未使用変数 `prc_banner` を削除 (単一プロセッサ構成では不使用)。
2. `print_banner_copyright` の未使用 `exinf` パラメータに `(void)exinf;` を追加。

---

### `asp3/sample/sample1.c`

1. 未使用変数 `cnt` を削除。
2. 未使用パラメータ `exinf` に `(void)exinf;` を追加 (4関数)。

   対象関数: `cyclic_handler`, `alarm_handler`, `exc_task`, `main_task`

---

### `asp3/library/log_output.c`

`c - '0'` (`int`) を `uint_t` へ代入する際の符号変換警告。

```c
width = width * 10U + (uint_t)(c - '0');
```

---

### `asp3/kernel/task.c`

`uint_t` を `uint8_t` フィールド (`priority`, `bpriority`) に代入する際の精度損失警告。

```c
p_tcb->bpriority = (uint8_t)p_tcb->p_tinib->ipriority;
p_tcb->priority  = (uint8_t)p_tcb->p_tinib->ipriority;
p_tcb->priority  = (uint8_t)newpri;
```

---

### `asp3/kernel/task_manage.c`

```c
p_tcb->bpriority = (uint8_t)newbpri;
```

---

### `asp3/kernel/mutex.c`

1. `bool_t` (`int`) を `BIT_FIELD_BOOL : 1` (`unsigned int`) に代入する符号変換警告。

   ```c
   p_tcb->boosted = (uint_t)boosted;
   ```

2. `const uint_t ceilpri` を `uint8_t priority` に代入する精度損失警告。

   ```c
   p_tcb->priority = (uint8_t)p_mtxcb->p_mtxinib->ceilpri;
   ```

---

### `asp3/kernel/mempfix.c`

ポインタ減算結果 (`ptrdiff_t`, 符号付き) を `size_t` (符号なし) に代入する符号変換警告。

```c
blkoffset = (size_t)(((char *) blk) - (char *)(p_mpfcb->p_mpfinib->mpf));
```

---

### `asp3/kernel/time_manage.c`

`int32_t adjtim` を `EVTTIM` (`unsigned int`) に加算する符号変換警告。

```c
current_evttim += (EVTTIM)adjtim;
```

---

## Step 6: EK-RA8M2 起動時シリアル文字化け修正

対象ボード: EK-RA8M2 (SCI_B = R_SCI_B_UART, SCI8, 115200 8N1)
変更ファイル: `asp3/target/ek_ra8m2/target_serial.c` のみ（カーネル・FSP 本体は未変更）

### 症状

TOPPERS/ASP3 起動バナー（`target_fput_log` によるポーリング送信）は正常表示。
その直後の割り込み駆動出力（"System logging task is started on port 1."）の
**先頭が文字化け**する。本文以降・"Sample program starts"・"task1 is running" は正常。
EK-RA6M5 (SCI = R_SCI_UART) では発生しない。

### 実機調査で判明した事実（J-Link で受信バイトを hex 取得して切り分け）

1. 文字化けは決定論的（同じバイト値が反復）であり，電気ノイズではなく
   **受信側 UART のフレーム同期ずれ**である。長いアイドルが来ると再同期して回復する。
2. 文字化けの実体は 2 つの独立要因の合成だった：
   - **(A) 再 Open グリッチ**（支配的・多バイトの化け）
   - **(B) TE セット時の 1 フレーム遅延**（先頭 1 バイト 0xFE）

#### (A) 再 Open グリッチ

`sio_opn_por` は割り込み駆動送信を確立するため `R_SCI_B_UART_Open` を呼ぶ。
本プロジェクトは `BSP_CFG_PARAM_CHECKING_ENABLE=0` のため，FSP の Open は
`ALREADY_OPEN` を返さず（その判定が条件コンパイルで無効），**全レジスタを再初期化**する。
その過程の `CCR0 = IDSEL`（TE/RE を一旦 0 にする）で TXD 線に短い LOW グリッチが乗り，
受信側が同期を失う。直後の最初のメッセージが広範囲に化ける。
この再 Open 自体は **送信確立に必須**（省くと TXI/TEI が NVIC で有効化されず，
割り込み駆動送信が始まらないことを実機で確認）。

#### (B) TE セット時の 1 フレーム遅延（残存 1 バイト 0xFE）

`R_SCI_B_UART_Write` は送信のたびに TE を 1→0→1 とトグルする。冷えた最初の送信での
TE セットは «TE セット時の 1 フレーム遅延»（RA ハードウェアマニュアル／RA6M5 FSP の
コメントが言及）により TXD に約 2 ビット幅の LOW を生じ，受信側がこれを 1 フレーム
(0xFE) として拾う。データ書き込みの有無に依らず CCR0|=TE だけで発生することを実機確認。
TE をトグルしない送信（下記）にしても (A) の再 Open グリッチ由来で 0xFE は残るため，
**この 1 バイトは target_serial.c 内の後処理だけでは除去不可**（除去には FSP の Open
改変等が必要だが本対応のスコープ外）。

### 修正内容

1. **再 Open グリッチからの再同期**: `sio_opn_por` で `R_SCI_B_UART_Open` 後に
   `sil_dly_nse(10ms)` で TXD を idle(High) のまま保持し，受信側を再同期させてから
   実データ送信に入る（起動時に一度だけ）。これで (A) 由来の多バイト化けが解消し，
   化けは先頭 1 バイト(0xFE)のみに縮退する（実機で 1ms→数バイト化け，10ms→1バイトを確認）。

2. **TE 非トグル送信（RA6M5 流）**: `sio_snd_chr` は `R_SCI_B_UART_Write` を使わず，
   TE は Open 時の 1 のまま維持し，グリッチのないポーリングと同様に TDR へ直接書き込んで
   送信し TIE を許可する。次バイトは `UART_EVENT_TX_DATA_EMPTY` を契機に供給する。
   FSP TXI ISR が送信完了で立てる TEIE を `target_uart_handler` のコールバックで
   クリアして `sci_b_uart_tei_isr` による TE クリアを抑止し，TE を一切トグルしない。
   （`tx_src_bytes` は 0 のままにし FSP TXI ISR には TDR を書かせない）

### 結果（実機 115200 8N1）

- バナー：正常
- "System logging task is started on port 1."：先頭に 0xFE が 1 バイトのみ，以降は完全クリーン
- "Sample program starts" / "task1 is running (NNN)."：完全クリーン
- 複数回リセットで安定（残存は常に 0xFE 1 バイトのみ）

文字化けは「先頭メッセージ全体＋広範囲」から「先頭 0xFE 1 バイトのみ」へ大幅低減。
残存 1 バイトは上記 (B)+(A) の SCI_B ハードウェア特性によるもので，
`target_serial.c` に閉じた対応の限界として確定（ユーザ合意済み）。
