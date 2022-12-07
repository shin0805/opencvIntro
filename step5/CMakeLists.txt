cmake_minimum_required(VERSION 2.8.3)
project(step5)
set(SOURCE_FILE main.cpp)
 
# OpenCVのビルド環境の設定ファイルを読み込む
find_package(OpenCV REQUIRED)  
 
# OpenCV関係のインクルードディレクトリのパスを設定
include_directories(
    ${OpenCV_INCLUDE_DIRS}
)
 
# 実行ファイル名を設定
add_executable(main
    ${SOURCE_FILE}
)
 
# リンクするライブラリを読み込む
target_link_libraries(main
    ${OpenCV_LIBRARIES}
    alut
    alure
    openal
)
