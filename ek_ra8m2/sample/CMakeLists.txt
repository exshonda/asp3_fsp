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
project(asp3_fsp
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

# TOPPERS/ASP3 のターゲット依存部を指定
# 変更点(1): 別ボードに移植する場合は asp3/target/ 以下のディレクトリ名に合わせる
#   例) EK-RA6M5 の場合: set(ASP3_TARGET ek_ra6m5)
set(ASP3_TARGET ek_ra8m2)

# TOPPERS/ASP3 の FSP 用のコンパイラ定義をインクルード
include(${CMAKE_CURRENT_LIST_DIR}/../../asp3/asp3_fsp.cmake)

target_link_libraries(${CMAKE_PROJECT_NAME}.elf asp3)

# TOPPERS/ASP3 のカーネルオブジェクト定義の cfg ファイルを設定
# 変更点(2): 自作アプリ用の .cfg ファイルに差し替える
#   例) set(ASP3_APP_CFG_FILE ${CMAKE_CURRENT_LIST_DIR}/src/myapp.cfg)
set(ASP3_APP_CFG_FILE ${ASP3_ROOT_DIR}/sample/sample1.cfg)

list(APPEND ASP3_INCLUDE_DIRS
    ${ASP3_ROOT_DIR}/sample
)

# TOPPERS/ASP3 のライブラリを追加
add_subdirectory(${ASP3_ROOT_DIR} asp3)

# TOPPERS/ASP3 付属のソースファイルを追加
# 変更点(3): アプリのソースファイルを自分の .c ファイルに差し替える
#   例) ${CMAKE_CURRENT_LIST_DIR}/src/myapp.c
include(${ASP3_ROOT_DIR}/library/library.cmake)
include(${ASP3_ROOT_DIR}/syssvc/syssvc.cmake)

target_sources(${CMAKE_PROJECT_NAME}.elf PRIVATE
    ${ASP3_ROOT_DIR}/sample/sample1.c
)

# TOPPERS/ASP3 を使うための FSP の設定
asp3_set_fsp_options(${CMAKE_PROJECT_NAME}.elf)

# TOPPERS/ASP3 のチェックを行う（シンボルが GC されるとエラーになるので省略）
#asp3_cfg_check(${CMAKE_PROJECT_NAME}.elf)

# Execute post-build to print size, generate hex and bin
get_filename_component(_TOOLCHAIN_BIN "${CMAKE_C_COMPILER}" DIRECTORY)
find_program(LLVM_SIZE NAMES llvm-size HINTS "${_TOOLCHAIN_BIN}" NO_DEFAULT_PATH)
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD
	COMMAND ${LLVM_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf>
	COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf> ${CMAKE_PROJECT_NAME}.hex
	COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf> ${CMAKE_PROJECT_NAME}.bin
)
