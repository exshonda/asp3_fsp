
list(APPEND ASP3_SYMVAL_TABLES
    ${ARCHDIR}/common/core_sym.def
)

list(APPEND ASP3_OFFSET_TRB_FILES
    ${ARCHDIR}/common/core_offset.trb
)

list(APPEND ASP3_INCLUDE_DIRS
    ${ARCHDIR}/ra6m5_fsp
    ${ARCHDIR}/common
    ${PROJECT_SOURCE_DIR}/arch/gcc
)

list(APPEND ASP3_COMPILE_DEFS
    TOPPERS_CORTEX_M33
    __TARGET_ARCH_THUMB=5
    __TARGET_FPU_FPV4_SP
    TOPPERS_ENABLE_TRUSTZONE
)

list(APPEND ASP3_ARCH_C_FILES
    ${ARCHDIR}/common/core_kernel_impl.c
    ${ARCHDIR}/common/core_support.S
)
set_property(SOURCE ${ARCHDIR}/common/core_support.S APPEND PROPERTY COMPILE_OPTIONS "${RASC_CMAKE_ASM_FLAGS}")

set(ARCH_SERIAL ${ARCHDIR}/ra6m5_fsp/chip_serial.c)
