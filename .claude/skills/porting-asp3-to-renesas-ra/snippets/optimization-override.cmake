# 最適化レベルのオーバーライド
#
# 問題: GeneratedCfg.cmake は RASC が自動生成するファイルで、
# RASC プレビルドで毎回上書きされるため直接編集できない。
# 例えば RASC v6.4.0 の RA8M2 用テンプレートには末尾に "-Os" が含まれる:
#
#     SET(RASC_CMAKE_C_FLAGS "...;-Os")
#
# これを -O2 に変更するには CMakeLists.txt 側で string(REPLACE ...) する。
#
# include(GeneratedCfg.cmake) の後・include(GeneratedSrc.cmake) の前 に書くこと。

string(REPLACE "-Os" "-O2" RASC_CMAKE_C_FLAGS   "${RASC_CMAKE_C_FLAGS}")
string(REPLACE "-Os" "-O2" RASC_CMAKE_CXX_FLAGS "${RASC_CMAKE_CXX_FLAGS}")

# 他にもオーバーライドしたいフラグがあれば同様にここで処理する。
# 例: 警告フラグを追加
# list(APPEND RASC_CMAKE_C_FLAGS "-Wundef")
#
# 例: 特定の警告を抑制（最終手段。原則ソースで直す）
# list(REMOVE_ITEM RASC_CMAKE_C_FLAGS "-Wconversion")
