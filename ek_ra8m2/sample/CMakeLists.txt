# Please see README.md for build instructions

cmake_minimum_required(VERSION 3.16.4)

set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_SYSTEM_PROCESSOR arm)

# Use add_library() with the STATIC option to name the source file in the generated project.
# This avoids running the linker and is intended for use with cross-compiling toolchains that
# cannot link without custom flags or linker scripts
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)

if(BUILD_KIT)
	message(NOTICE "Build Kit is defined as ${BUILD_KIT}")
endif()

message(NOTICE "CMAKE_CURRENT_BINARY_DIR is defined as ${CMAKE_CURRENT_BINARY_DIR}")

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(isMultiConfig)
    message(NOTICE "Multi-config generator(${CMAKE_GENERATOR}) is active")
else()
    message(NOTICE "Single-config generator(${CMAKE_GENERATOR}) is active")
	if(DEFINED CMAKE_BUILD_TYPE)
	 	message(NOTICE "CMAKE_BUILD_TYPE is defined as ${CMAKE_BUILD_TYPE}")
	elseif(DEFINED BUILD_TYPE)
		set(CMAKE_BUILD_TYPE ${BUILD_TYPE})
		message(NOTICE "CMAKE_BUILD_TYPE is set to BUILD_TYPE ENV ${CMAKE_BUILD_TYPE}")
	else()
		message(FATAL_ERROR "Please specify the CMAKE_BUILD_TYPE with -DCMAKE_BUILD_TYPE=")
	endif()
    #set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
	message(NOTICE "CMAKE_RUNTIME_OUTPUT_DIRECTORY is defined as ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

cmake_minimum_required(VERSION 3.16.4)

# Project configuration
project(sample
	VERSION 1.0.0
	LANGUAGES C CXX ASM)

set(CMAKE_CROSSCOMPILING "TRUE")
set(CMAKE_EXPORT_COMPILE_COMMANDS "TRUE")

include(${CMAKE_CURRENT_LIST_DIR}/Config.cmake)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GeneratedCfg.cmake)

# Override optimization level: -Os -> -O2
string(REPLACE "-Os" "-O2" RASC_CMAKE_C_FLAGS   "${RASC_CMAKE_C_FLAGS}")
string(REPLACE "-Os" "-O2" RASC_CMAKE_CXX_FLAGS "${RASC_CMAKE_CXX_FLAGS}")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GeneratedSrc.cmake)

# TOPPERS/ASP3 縺ｮ繧ｿ繝ｼ繧ｲ繝・ヨ萓晏ｭ倬Κ繧呈欠螳・
set(ASP3_TARGET ek_ra8m2)

# TOPPERS/ASP3 縺ｮ FSP 逕ｨ縺ｮ繧ｳ繝ｳ繝代う繝ｩ螳夂ｾｩ繧偵う繝ｳ繧ｯ繝ｫ繝ｼ繝・
include(${CMAKE_CURRENT_LIST_DIR}/../../asp3/asp3_fsp.cmake)

target_link_libraries(${CMAKE_PROJECT_NAME}.elf asp3)

# TOPPERS/ASP3 縺ｮ繧ｫ繝ｼ繝阪Ν繧ｪ繝悶ず繧ｧ繧ｯ繝亥ｮ夂ｾｩ縺ｮcfg繝輔ぃ繧､繝ｫ繧定ｨｭ螳・
set(ASP3_APP_CFG_FILE ${ASP3_ROOT_DIR}/sample/sample1.cfg)

list(APPEND ASP3_INCLUDE_DIRS
    ${ASP3_ROOT_DIR}/sample
)

# TOPPERS/ASP3 縺ｮ繝ｩ繧､繝悶Λ繝ｪ繧定ｿｽ蜉
add_subdirectory(${ASP3_ROOT_DIR} asp3)

# TOPPERS/ASP3 莉伜ｱ槭・繧ｽ繝ｼ繧ｹ繝輔ぃ繧､繝ｫ繧定ｿｽ蜉
include(${ASP3_ROOT_DIR}/library/library.cmake)
include(${ASP3_ROOT_DIR}/syssvc/syssvc.cmake)

target_sources(${CMAKE_PROJECT_NAME}.elf PRIVATE
    ${ASP3_ROOT_DIR}/sample/sample1.c
)

# TOPPERS/ASP3 繧剃ｽｿ縺・◆繧√・ FSP 縺ｮ險ｭ螳・
asp3_set_fsp_options(${CMAKE_PROJECT_NAME}.elf)

# TOPPERS/ASP3 縺ｮ繝√ぉ繝・け繧定｡後≧・医す繝ｳ繝懊Ν縺隈C縺輔ｌ繧九→繧ｨ繝ｩ繝ｼ縺ｫ縺ｪ繧九・縺ｧ逵∫払・・
#asp3_cfg_check(${CMAKE_PROJECT_NAME}.elf)

# Execute post-build to print size, generate hex and bin
get_filename_component(_TOOLCHAIN_BIN "${CMAKE_C_COMPILER}" DIRECTORY)
find_program(LLVM_SIZE NAMES llvm-size HINTS "${_TOOLCHAIN_BIN}" NO_DEFAULT_PATH)
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD
	COMMAND ${LLVM_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf>
	COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf> ${CMAKE_PROJECT_NAME}.hex
	COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf> ${CMAKE_PROJECT_NAME}.bin
)
