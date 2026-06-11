#
#  外部（SDK）ターゲットのパス解決規約（asp3_core PORTING_GUIDE「外部ターゲット」）：
#   - 共通arch（arch/arm_m_gcc/common）は asp3_core サブモジュール側＝ASP3_ROOT_DIR
#   - チップ依存部（ra8m2_fsp）・ターゲット依存部は本リポジトリ側＝CMAKE_CURRENT_LIST_DIR 相対
#
set(ARCHDIR ${ASP3_ROOT_DIR}/arch/arm_m_gcc)
get_filename_component(CHIPDIR ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm_m_gcc/ra8m2_fsp ABSOLUTE)
set(TARGETDIR ${CMAKE_CURRENT_LIST_DIR})

list(APPEND ASP3_CFG_FILES
    ${TARGETDIR}/target_kernel.cfg
)

list(APPEND ASP3_KERNEL_CFG_TRB_FILES
    ${TARGETDIR}/target_kernel.py
)

list(APPEND ASP3_CHECK_TRB_FILES
    ${TARGETDIR}/target_check.py
)

list(APPEND ASP3_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/ra/arm/CMSIS_6/CMSIS/Core/Include
    ${CMAKE_SOURCE_DIR}/ra/fsp/inc
    ${CMAKE_SOURCE_DIR}/ra/fsp/inc/api
    ${CMAKE_SOURCE_DIR}/ra/fsp/inc/instances
    ${CMAKE_SOURCE_DIR}/ra_cfg/fsp_cfg
    ${CMAKE_SOURCE_DIR}/ra_cfg/fsp_cfg/bsp
    ${CMAKE_SOURCE_DIR}/ra_gen
    ${TARGETDIR}
)

list(APPEND ASP3_COMPILE_DEFS
    RA8M2
    $<$<CONFIG:Debug>:DEBUG>
    USE_TIM_AS_HRT
    TOPPERS_FPU_ENABLE
    TOPPERS_FPU_LAZYSTACKING
    TOPPERS_FPU_CONTEXT
)

list(APPEND ASP3_TARGET_C_FILES
    ${TARGETDIR}/target_kernel_impl.c
    ${TARGETDIR}/target_timer.c
    ${TARGETDIR}/target_serial.c
)

#  FSP/RASC のフラグ・定義・include を asp3 lib と cfg1_out へ（ek_ra6m5 と同方針）
list(APPEND ASP3_COMPILE_OPTIONS ${RASC_CMAKE_C_FLAGS})
list(APPEND ASP3_COMPILE_DEFS    ${RASC_CMAKE_DEFINITIONS})
list(APPEND ASP3_INCLUDE_DIRS    ${CMAKE_SOURCE_DIR})

#  cfg1_out（使い捨てELF）の最小リンク：arch（Cortex-M85/fpv5-d16）＋ crt0回避。
#  FSP実リンカスクリプトは不要（nm でのシンボル値抽出のみ）。
list(APPEND ASP3_LINK_OPTIONS
    --target=arm-none-eabi -mcpu=cortex-m85 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
    -nostartfiles -nostdlib)
list(APPEND ASP3_LINK_LIBS c)

include(${CHIPDIR}/arch.cmake)

#  cfg1_out の RASC生成依存はアプリ側 CMakeLists の add_subdirectory(asp3_core) 後に記述。
