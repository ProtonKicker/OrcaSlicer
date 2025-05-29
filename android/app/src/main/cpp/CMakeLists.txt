# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.13)

# 设置项目名称
project(orcaslicer)

# 添加预处理器定义
add_definitions(-DSLIC3R_GUI)
add_definitions(-D__ANDROID__)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置源代码根目录
set(ORCA_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)

# 包含OrcaSlicer的主要源代码目录
include_directories(
    ${ORCA_ROOT_DIR}/src
    ${ORCA_ROOT_DIR}/src/libslic3r
    ${ORCA_ROOT_DIR}/src/slic3r
    ${ORCA_ROOT_DIR}/src/slic3r/GUI
)

# 添加JNI和Android特定的源文件
set(ANDROID_SOURCES
    OrcaSlicerJNI.cpp
    OrcaSlicerAndroid.cpp
)

# 添加OrcaSlicer的Android特定实现
set(ORCA_ANDROID_SOURCES
    ${ORCA_ROOT_DIR}/src/slic3r/GUI/OpenGLManager_Android.cpp
)

# 添加OrcaSlicer的核心源文件
# 注意：这里只是一个示例，实际需要包含所有必要的源文件
set(ORCA_CORE_SOURCES
    # 在这里添加OrcaSlicer的核心源文件
    # 例如：
    # ${ORCA_ROOT_DIR}/src/libslic3r/Point.cpp
    # ${ORCA_ROOT_DIR}/src/libslic3r/Polygon.cpp
    # 等等...
)

# 添加OrcaSlicer的GUI源文件
# 注意：这里只是一个示例，实际需要包含所有必要的源文件
set(ORCA_GUI_SOURCES
    # 在这里添加OrcaSlicer的GUI源文件
    # 例如：
    # ${ORCA_ROOT_DIR}/src/slic3r/GUI/GLCanvas3D.cpp
    # ${ORCA_ROOT_DIR}/src/slic3r/GUI/GLShader.cpp
    # 等等...
)

# 创建共享库
add_library(
    orcaslicer
    SHARED
    ${ANDROID_SOURCES}
    ${ORCA_ANDROID_SOURCES}
    # 暂时注释掉，直到我们准备好包含所有必要的源文件
    # ${ORCA_CORE_SOURCES}
    # ${ORCA_GUI_SOURCES}
)

# 链接Android特定的库
target_link_libraries(
    orcaslicer
    android
    log
    EGL
    GLESv3
)

# 设置编译选项
target_compile_options(
    orcaslicer
    PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-sign-compare
)

# 设置包含目录
target_include_directories(
    orcaslicer
    PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
)