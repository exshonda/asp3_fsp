#
#  外部（SDK）ターゲットのパス解決規約（asp3_core PORTING_GUIDE「外部ターゲット」）：
#   - 共通arch（arch/arm_m_gcc/common）は asp3_core サブモジュール側＝ASP3_ROOT_DIR
#   - チップ依存部（ra6m5_fsp）・ターゲット依存部は本リポジトリ側＝CMAKE_CURRENT_LIST_DIR 相対
#     （本ファイルは asp3_core の CMakeLists から ASP3_TARGET_DIR 経由で include されるため）
#
set(ARCHDIR ${ASP3_ROOT_DIR}/arch/arm_m_gcc)
get_filename_component(CHIPDIR ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm_m_gcc/ra6m5_fsp ABSOLUTE)
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
    RA6M5
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

#
#  FSP/RASC のコンパイルフラグ・定義・インクルードを asp3 ライブラリと cfg1_out へ供給。
#  asp3_core の CMakeLists は ASP3_COMPILE_OPTIONS / ASP3_COMPILE_DEFS /
#  ASP3_INCLUDE_DIRS を asp3 lib（PUBLIC）と cfg1_out の両方へ適用する。
#  RASC_CMAKE_* は GeneratedCfg.cmake が定義する CMake リスト（`;` 区切り）で、
#  アプリ側 CMakeLists で -Os→-O2 置換済み・add_subdirectory の親スコープから継承される。
#   - RASC_CMAKE_C_FLAGS : --target=arm-none-eabi / -mcpu=cortex-m33 / -mfpu 等（arch必須）
#   - RASC_CMAKE_DEFINITIONS : _RA_CORE=CM33 等
#   - ${CMAKE_SOURCE_DIR} : bsp_linker_info.h 等（アプリ直下の生成/設定ヘッダ）
#
list(APPEND ASP3_COMPILE_OPTIONS ${RASC_CMAKE_C_FLAGS})
list(APPEND ASP3_COMPILE_DEFS    ${RASC_CMAKE_DEFINITIONS})
list(APPEND ASP3_INCLUDE_DIRS    ${CMAKE_SOURCE_DIR})

#  cfg1_out（静的API値抽出用の使い捨てELF）のリンク設定。
#  ASP3_LINK_OPTIONS / ASP3_LINK_LIBS は asp3_core で cfg1_out のリンクに使われる
#  （ライブラリ専用モードでは asp 実行ファイルは作らないため cfg1_out 専用）。
#  cfg1_out は nm でシンボル値を読むだけなので FSP の実リンカスクリプト
#  （fsp.lld / asp3_sections.lld）は不要。arch（--target/-mcpu：正しい multilib 選択用）と
#  -nostartfiles（ATfE picolibc の crt0 回避）・-nostdlib のみ与え、既定レイアウトでリンクする。
list(APPEND ASP3_LINK_OPTIONS
    --target=arm-none-eabi -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb
    -nostartfiles -nostdlib)
list(APPEND ASP3_LINK_LIBS c)

include(${CHIPDIR}/arch.cmake)

#  cfg1_out（FSP生成ヘッダ ra/・ra_gen/ を要する）の RASC 生成依存は、
#  canonical asp3_core では cfg1_out ターゲットが本ファイル include 後に生成されるため、
#  ここではなくアプリ側 CMakeLists の add_subdirectory(asp3_core) 後に
#  add_dependencies(cfg1_out generate_content_<proj>) を記述する
#  （asp3_core PORTING_GUIDE「別ツールチェーンの併用」の注記）。
