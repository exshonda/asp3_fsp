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
