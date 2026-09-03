cmake_minimum_required(VERSION 3.15)
project(FortuneRenderer LANGUAGES CXX)
 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
 
# 项目本身的源文件：
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_SOURCE_DIR}/source/*.h"
    "${CMAKE_SOURCE_DIR}/source/*.hpp"
)
 
add_executable(FortuneRenderer ${APP_SOURCES})

# MSVC：统一按 UTF-8 解析源码（源文件为 UTF-8 无 BOM 的中文注释，避免 C4819 警告）
if (MSVC)
    target_compile_options(FortuneRenderer PRIVATE /utf-8)
endif()
 
# 头文件附加目录：minifb/include 和 glm
target_include_directories(FortuneRenderer PRIVATE
    "${CMAKE_SOURCE_DIR}/minifb/include"
    "${CMAKE_SOURCE_DIR}/glm"
)
 
# 添加 minifb 到构建项目：
add_subdirectory(minifb)

target_link_libraries(FortuneRenderer PRIVATE minifb)
 
if (WIN32)

# 链接库：
target_link_libraries(FortuneRenderer PRIVATE
    opengl32
    winmm
    kernel32
    user32
    gdi32
    winspool
    shell32
    ole32
    oleaut32
    uuid
    comdlg32
    advapi32)

endif()

