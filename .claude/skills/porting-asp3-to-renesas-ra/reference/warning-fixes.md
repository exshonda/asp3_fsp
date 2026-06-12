# ビルド警告の典型パターンと対処

`-Wall -Wextra -Wconversion`（FSP のデフォルト）で警告ゼロを達成するための定型対処集。
**原則として `-Wno-*` での抑制はせず、ソースを直す**こと。

> **編集先の注意（submodule 構成）**：警告がカーネル・共通arch
> （`asp3/asp3_core/` 配下＝kernel/・include/・arch/arm_m_gcc/common/ 等）にある場合、
> 本リポジトリからは編集しない。asp3_core リポジトリ側で修正して submodule を更新する
> （同リポジトリ AGENTS.md の禁則に従う。`EXC_RETURN_PREFIX` の `#ifndef` ガード等は
> asp3_core main に取込済み）。本ファイルの対処を直接適用してよいのは FSP 固有部
> （`asp3/arch/arm_m_gcc/ra*_fsp/`・`asp3/target/`・`ek_ra*/sample/`）のみ。

## INT 昇格による型不一致

C の整数昇格で `uint16_t` 同士の演算が `int` に昇格する。戻り値を `uint16_t` に代入する場面でキャストが必要。

```c
// 警告: implicit conversion changes signedness
*p = TOPPERS_SIL_REV_ENDIAN_UINT16(x);

// OK
*p = (uint16_t)TOPPERS_SIL_REV_ENDIAN_UINT16(x);
```

## `~` 演算子の符号変換

`~` は `int` 引数で `int` を返すため、`uint32_t` への代入で警告になる。

```c
// 警告
tmp &= ~CCR_STKALIGN;
tmp &= ~(0xFF << shift);

// OK
tmp &= ~(uint32_t)CCR_STKALIGN;
tmp &= ~(0xFFU << shift);
```

## `1 <<` のシフト

`1` は `int`。負数になる可能性のあるシフトで警告。

```c
// 警告
mask = 1 << n;

// OK
mask = 1U << n;
```

## ビットフィールド / `uint8_t` メンバーへの代入

`uint_t`（≒ `unsigned int`）から `uint8_t` への代入は精度損失警告。

```c
// 警告
p_tcb->priority = newpri;

// OK
p_tcb->priority = (uint8_t)newpri;
```

## `bool_t` から bit-field への代入

`bool_t`（実体 `int`）から `unsigned : 1` ビットフィールドへの代入も符号変換扱い。

```c
// 警告
p_tcb->boosted = some_bool;

// OK
p_tcb->boosted = (uint_t)some_bool;
```

## `ptrdiff_t` から `size_t` への代入

ポインタ減算結果は符号付き。サイズ系の変数に代入するときキャストする。

```c
// 警告
size_t offset = ((char *)p_end) - ((char *)p_start);

// OK
size_t offset = (size_t)(((char *)p_end) - ((char *)p_start));
```

## CMSIS との二重定義

`EXC_RETURN_PREFIX` 等が CMSIS の `core_cm<NN>.h` でも定義されている場合は `#ifndef` でガード。

```c
#ifndef EXC_RETURN_PREFIX
#define EXC_RETURN_PREFIX 0xff000000
#endif
```

## 未使用パラメータ（callback で頻出）

`exinf` 引数を使わないコールバックは `(void)exinf;` を入れる。

```c
void syslog_initialize(intptr_t exinf) {
    (void)exinf;
    // ...
}
```

`-Wunused-parameter` は標準で有効なので、関数を空にしただけでは警告が出る。

## アセンブリから参照されるマクロ

`INT_IPM` のように `#IIPM_LOCK` 形式でアセンブリからも使われるマクロ本体は変更不可。
**C 側の呼び出し箇所だけにキャスト** する。

```c
// 警告（INT_IPM の戻り値が int）
set_basepri_max(IIPM_LOCK);
const uint32_t iipm = INT_IPM(intpri);

// OK
set_basepri_max((uint32_t)IIPM_LOCK);
const uint32_t iipm = (uint32_t)INT_IPM(intpri);
```

## 未使用ローカル変数

戻り値を捨てるだけなら `(void)` キャストでよい。

```c
// 警告
uint32_t status;
status = R_SCI_UART_Open(&ctrl, &cfg);  // status を使わない

// OK
(void)R_SCI_UART_Open(&ctrl, &cfg);
```

## 構造体初期化子

スカラーメンバーに `{0}` を使うと警告。逆に配列に `0` だけ書くのも警告。

```c
typedef struct {
    int      a;        // スカラー
    char     buf[16];  // 配列
} S;

// 警告: スカラーに {0}
S s = {0, {0}};

// OK
S s = {0, "\0"};   // または s = {0}; で全ゼロ初期化
```

## ループ変数の符号比較

```c
int i;
for (i = 0; i < tnum_tsk; ++i) { ... }  // 警告: int vs uint_t

// OK
for (uint_t i = 0; i < tnum_tsk; ++i) { ... }
```
