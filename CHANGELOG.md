# Changelog

## Unreleased

### Fixed

- ARM Toolchain for Embedded (ATfE) 21.1.1 でビルドできるよう以下を修正
  （EK-RA8M2 / EK-RA6M5 サンプル両方）
  - clang が picolibc の `crt0.o` を引き込み、その `_start` が
    `__data_size` / `__bss_start` 等を要求するためリンクエラーになる問題を、
    `RASC_CMAKE_EXE_LINKER_FLAGS` に `-nostartfiles` を追加して回避
    （ASP3+FSP のエントリは `Reset_Handler` なので crt0 は不要）
  - RASC 生成の `script/fsp.lld` (`fsp_gen.lld`) が ASP3 固有セクション
    (`.vector` / `.empty.*`) を認識せず、ld.lld のデフォルト orphan
    placement で最後の memory region (`OPTION_SETTING_OTP_ZHUK`) に
    配置されてリンクエラーになる問題を、追加リンカスクリプト
    `cmake/asp3_sections.lld` を `-T` で渡し、`INSERT AFTER
    __flash_readonly$$` で FLASH 内に明示配置することで回避
