
set(ARCHDIR ${PROJECT_SOURCE_DIR}/arch/arm_m_gcc)
set(TARGETDIR ${PROJECT_SOURCE_DIR}/target/${ASP3_TARGET})

list(APPEND ASP3_CFG_FILES
    ${TARGETDIR}/target_kernel.cfg
)

list(APPEND ASP3_KERNEL_CFG_TRB_FILES
    ${TARGETDIR}/target_kernel.trb
)

list(APPEND ASP3_CHECK_TRB_FILES
    ${TARGETDIR}/target_check.trb
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

include(${ARCHDIR}/ra6m5_fsp/arch.cmake)
