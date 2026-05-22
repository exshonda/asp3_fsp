# CMAKE_SIZE 未定義バグの修正スニペット
#
# 問題: RASC が生成する CMakeLists.txt テンプレートには以下が含まれる:
#
#     add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD
#         COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf>
#         ...
#     )
#
# しかし CMAKE_SIZE は LLVM toolchain でも RASC でも定義されていないため、
# `COMMAND ${CMAKE_SIZE} sample.elf` が `COMMAND sample.elf` に展開され、
# Windows がビルドのたびに ELF を「開く」状態になる（既定アプリで起動する）。
#
# 修正: CMAKE_C_COMPILER と同じディレクトリから llvm-size を探して使う。

get_filename_component(_TOOLCHAIN_BIN "${CMAKE_C_COMPILER}" DIRECTORY)
find_program(LLVM_SIZE NAMES llvm-size HINTS "${_TOOLCHAIN_BIN}" NO_DEFAULT_PATH)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD
    COMMAND ${LLVM_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf>
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf> ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}.elf> ${CMAKE_PROJECT_NAME}.bin
)
