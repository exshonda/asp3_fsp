# Please see README.md for build instructions

cmake_minimum_required(VERSION 3.16.4)

set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_SYSTEM_PROCESSOR arm)

# クロスコンパイラの動作確認テストでリンクを行わない（ARM向けclangはホスト用CRTを持たないため）
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

# ATfE 21.x の clang は既定で picolibc の crt0.o を引き込み、その _start が
# __data_size/__bss_start などを参照するが、ASP3+FSP のリンカスクリプトは
# これらを定義していない（エントリは Reset_Handler）。crt0 等のスタートアップ
# ファイルをリンクから除外する。
list(APPEND RASC_CMAKE_EXE_LINKER_FLAGS "-nostartfiles")

# RASC が生成する script/fsp.lld (fsp_gen.lld) は ASP3 固有セクション
# (.vector, .empty.*) を認識しないため、ld.lld のデフォルト orphan placement
# で最後の memory region に配置されてリンクエラーになる。
# 追加リンカスクリプトを -T で渡し、FLASH 内に明示配置する。
list(APPEND RASC_CMAKE_EXE_LINKER_FLAGS
    "-T" "${CMAKE_CURRENT_LIST_DIR}/cmake/asp3_sections.lld")

# RASCで生成されるra/・ra_gen/が無いとGeneratedSrc.cmakeのGLOBが空になり
# 1回目のビルドでソース不足によるリンクエラーが発生するため、
# configure時にRASCを実行してコンテンツを先に生成しておく
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ra" OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ra_gen")
    message(STATUS "Generating FSP content using RASC (initial configure)...")
    execute_process(
        COMMAND ${RASC_EXE_PATH} -nosplash --launcher.suppressErrors --generate
                --devicefamily ra --compiler LLVMARM
                --toolchainversion ${CMAKE_C_COMPILER_VERSION}
                ${CMAKE_CURRENT_SOURCE_DIR}/configuration.xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE _rasc_result
    )
    if(NOT _rasc_result EQUAL 0)
        message(FATAL_ERROR "RASC content generation failed (exit ${_rasc_result}). Check RASC_EXE_PATH=${RASC_EXE_PATH}")
    endif()
    # ビルド時のpre-buildカスタムコマンドが直後に再実行されないようstampを作成
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/configuration.xml.stamp)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GeneratedSrc.cmake)

# ===== TOPPERS/ASP3 Core 取り込み（submodule＋ライブラリ専用モード） =====
# 旧 bundled asp3/(fork) を廃止し、純カーネル asp3_core を submodule 参照する構成。
# 経緯：asp3/asp3_core/docs/dev/fsp-integration.md（ek_ra6m5 と同型）

set(ASP3_MAIN_PROJECT_NAME ${CMAKE_PROJECT_NAME})

# ターゲット依存部（別ボードは asp3/target/ のディレクトリ名に合わせる）
set(ASP3_TARGET ek_ra8m2)

# FSP 協調ヘルパ（ASP3_TARGET_DIR / ASP3_CORE_DIR / ASP3_ROOT_DIR を解決）
include(${CMAKE_CURRENT_LIST_DIR}/../../asp3/asp3_fsp.cmake)

# アプリ（タスク定義）は asp3_core 同梱 sample（自作アプリに差し替え可）
set(ASP3_APPLDIR  ${ASP3_CORE_DIR}/sample)
set(ASP3_APPLNAME sample1)

# asp3 カーネルライブラリをライブラリ専用モードでビルド
set(ASP3_LIBRARY_ONLY ON CACHE BOOL "build asp3 as library only" FORCE)
add_subdirectory(${ASP3_CORE_DIR} asp3)

# 最終実行ファイルに asp3・タスク本体・システムサービスを追加
target_link_libraries(${CMAKE_PROJECT_NAME}.elf asp3)
target_sources(${CMAKE_PROJECT_NAME}.elf PRIVATE
    ${ASP3_CORE_DIR}/sample/sample1.c
)
asp3_add_syssvc(${CMAKE_PROJECT_NAME}.elf)
target_include_directories(${CMAKE_PROJECT_NAME}.elf PRIVATE
    ${ASP3_CORE_DIR}/sample
)

# cfg1_out を RASC 生成（FSPヘッダ ra/・ra_gen/）に依存させる（add_subdirectory 後）
if(TARGET generate_content_${ASP3_MAIN_PROJECT_NAME})
    add_dependencies(cfg1_out generate_content_${ASP3_MAIN_PROJECT_NAME})
endif()

# TOPPERS/ASP3 を使うための FSP の設定（現状なし）
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
