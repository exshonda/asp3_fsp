# -*- coding: utf-8 -*-
#
#   TOPPERS/ASP Kernel
#       Toyohashi Open Platform for Embedded Real-Time Systems/
#       Advanced Standard Profile Kernel
#
#   $Id: target_kernel.py (converted from target_kernel.trb) $
#

#
#     パス2のターゲット依存テンプレート（EK-RA8M2 ASP3用）
#

#
#  有効な割込み番号，割込みハンドラ番号
#
INTNO_VALID = list(range(15, TMAX_INTNO + 1))
INHNO_VALID = INTNO_VALID

#
#  有効なCPU例外番号
#
EXCNO_VALID = [2, 3, 4, 5, 6, 7, 12]

#
#  CRE_ISRで使用できる割込み番号とそれに対応する割込みハンドラ番号
#
INTNO_CREISR_VALID = INTNO_VALID
INHNO_CREISR_VALID = INHNO_VALID

#
#  DEF_INT／DEF_EXCで使用できる割込みハンドラ番号／CPU例外ハンドラ番号
#
INHNO_DEFINH_VALID = INHNO_VALID
EXCNO_DEFEXC_VALID = EXCNO_VALID

#
#  CFG_INTで使用できる割込み番号と割込み優先度
#  最大優先度はBASEPRIレジスタでマスクできない優先度（内部優先度'0'）
#  そのため，カーネル管理外の割込みでのみ指定可能．
INTNO_CFGINT_VALID = INTNO_VALID
INTPRI_CFGINT_VALID = list(range(-(1 << TBITW_IPRI), 0))

#
#  kernel/kernel.tf のターゲット依存部
#

#
#  TSKINICTXBの初期化情報を生成
#
def GenerateTskinictxb(key, params):
    return ("{" +
            f"\t(void *)({params['tinib_stk']}), " +
            f"\t((void *)((char *)({params['tinib_stk']}) + " +
            f"({params['tinib_stksz']}))), " +
            "},")

#
#  ベクタテーブルの予約領域はデフォルトで0にする
#
if 'GenResVectVal' not in locals():
    GenResVectVal = lambda num: 0

#
#  標準テンプレートファイルのインクルード
#
IncludeTrb("kernel/kernel.py")

kernelCfgC.append("""
/*
 *  Target-dependent Definitions (ARM-M)
 */

/*
 *  ベクターテーブル
 */
__attribute__ ((section(".vector"), aligned(0x80)))
const FP _kernel_vector_table[] = {
    (FP)(&g_main_stack[0] + BSP_CFG_STACK_MAIN_BYTES), /* 0 The initial stack pointer */
    (FP)Reset_Handler,                 /* 1 The reset handler */
""")

for excno in range(2, 15):
    if excno == 8:
        kernelCfgC.add(f"    (FP)({GenResVectVal(8)}),")
    elif excno == 9:
        kernelCfgC.add(f"    (FP)({GenResVectVal(9)}),")
    elif excno == 10:
        kernelCfgC.add(f"    (FP)({GenResVectVal(10)}),")
    elif excno == 11:
        kernelCfgC.add("    (FP)(_kernel_svc_handler),      /* 11 SVCall handler */")
    elif excno == 13:
        kernelCfgC.add(f"    (FP)({GenResVectVal(13)}),")
    elif excno == 14:
        kernelCfgC.add("    (FP)(_kernel_pendsv_handler),      /* 14 PendSV handler */")
    else:
        exc = cfgData.get('DEF_EXC', {}).get(excno)
        if exc and (exc.get('excatr', 0) & TA_DIRECT) != 0:
            kernelCfgC.add(f"    (FP)({exc['exchdr']}), /* {excno} */")
        else:
            kernelCfgC.add(f"    (FP)(_kernel_core_exc_entry), /* {excno} */")

for inhno in INTNO_VALID:
    inh = cfgData.get('DEF_INH', {}).get(inhno)
    if inh and (inh.get('inhatr', 0) & TA_NONKERNEL) != 0:
        kernelCfgC.add(f"    (FP)({inh['inthdr']}), /* {inhno} */")
    else:
        kernelCfgC.add(f"    (FP)(_kernel_core_int_entry), /* {inhno} */")

kernelCfgC.add2("};")

#
#  _kernel_bitpat_cfgintの生成
#

bitpat_cfgint_num = 0
if (TMAX_INTNO & 0x0f) == 0x00:
    bitpat_cfgint_num = (TMAX_INTNO >> 4)
else:
    bitpat_cfgint_num = (TMAX_INTNO >> 4) + 1

kernelCfgC.add("")
kernelCfgC.add(f"const uint32_t _kernel_bitpat_cfgint[{bitpat_cfgint_num}] = {{")
for num in range(bitpat_cfgint_num):
    bitpat_cfgint = 0
    for inhno in range(num * 32, (num * 32) + 32):
        inh_list = [v for k, v in cfgData.get('DEF_INH', {}).items() if v.get('inhno') == inhno]
        if inh_list:
            bitpat_cfgint |= (1 << (inhno & 0x01f))
    kernelCfgC.add(f"   UINT32_C(0x{bitpat_cfgint:08x}),")
kernelCfgC.add2("};")
