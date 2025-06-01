# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.10.2)

# 设置项目名称
project(orcaslicer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加Android预处理器定义
add_definitions(-DANDROID -DSLIC3R_GUI -DSLIC3R_ANDROID)

# 设置OrcaSlicer源码根目录
set(ORCA_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)

# 包含源目录
include_directories(
    ${ORCA_ROOT}/src
    ${ORCA_ROOT}/src/slic3r
    ${ORCA_ROOT}/src/libslic3r
    ${ORCA_ROOT}/src/cereal/include
    ${ORCA_ROOT}/src/clipper
    ${ORCA_ROOT}/src/boost
    ${ORCA_ROOT}/src/eigen
    ${ORCA_ROOT}/src/expat
    ${ORCA_ROOT}/src/imgui
    ${ORCA_ROOT}/src/miniz
    ${ORCA_ROOT}/src/nlopt
    ${ORCA_ROOT}/src/qhull
    ${ORCA_ROOT}/src/tbb/include
    ${ORCA_ROOT}/src/libigl/include
    ${ORCA_ROOT}/src/glu-libtess
    ${ORCA_ROOT}/src/poly2tri
    ${ORCA_ROOT}/src/polypartition
    ${ORCA_ROOT}/src/polyclipping
    ${ORCA_ROOT}/src/admesh
    ${ORCA_ROOT}/src/avrdude
    ${ORCA_ROOT}/src/semver
    ${ORCA_ROOT}/src/libslic3r/format
    ${ORCA_ROOT}/src/hidapi/include
    ${ORCA_ROOT}/src/qoi
    ${ORCA_ROOT}/src/libnest2d/include
    ${ORCA_ROOT}/src/Shiny
    ${ANDROID_NDK}/sources/android/native_app_glue
)

# 添加Android特定的源文件
set(ANDROID_SOURCES
    OrcaSlicerJNI.cpp
    OrcaSlicerAndroid.cpp
    OpenGLManager_Android.cpp
)

# 添加libslic3r核心源文件
file(GLOB_RECURSE LIBSLIC3R_SOURCES
    ${ORCA_ROOT}/src/libslic3r/*.cpp
    ${ORCA_ROOT}/src/libslic3r/Format/*.cpp
    ${ORCA_ROOT}/src/libslic3r/Technologies/*.cpp
    ${ORCA_ROOT}/src/libslic3r/Geometry/*.cpp
    ${ORCA_ROOT}/src/libslic3r/Utils/*.cpp
)

# 添加GUI源文件
file(GLOB_RECURSE GUI_SOURCES
    ${ORCA_ROOT}/src/slic3r/GUI/*.cpp
    ${ORCA_ROOT}/src/slic3r/Utils/*.cpp
)

# 排除不需要的GUI源文件
list(FILTER GUI_SOURCES EXCLUDE REGEX ".*_Desktop\.cpp$")
list(FILTER GUI_SOURCES EXCLUDE REGEX ".*_MSW\.cpp$")
list(FILTER GUI_SOURCES EXCLUDE REGEX ".*_OSX\.cpp$")
list(FILTER GUI_SOURCES EXCLUDE REGEX ".*_Linux\.cpp$")
list(FILTER GUI_SOURCES EXCLUDE REGEX ".*_Unix\.cpp$")
list(FILTER GUI_SOURCES EXCLUDE REGEX ".*wxWidgets.*\.cpp$")

# 添加第三方库源文件
file(GLOB ADMESH_SOURCES ${ORCA_ROOT}/src/admesh/*.c)
file(GLOB CLIPPER_SOURCES ${ORCA_ROOT}/src/clipper/*.cpp)
file(GLOB POLYPARTITION_SOURCES ${ORCA_ROOT}/src/polypartition/*.cpp)
file(GLOB POLY2TRI_SOURCES 
    ${ORCA_ROOT}/src/poly2tri/common/*.cc
    ${ORCA_ROOT}/src/poly2tri/sweep/*.cc
)
file(GLOB EXPAT_SOURCES ${ORCA_ROOT}/src/expat/*.c)
file(GLOB MINIZ_SOURCES ${ORCA_ROOT}/src/miniz/*.c)
file(GLOB SEMVER_SOURCES ${ORCA_ROOT}/src/semver/*.c)
file(GLOB GLULIBTESS_SOURCES ${ORCA_ROOT}/src/glu-libtess/*.c)
file(GLOB IMGUI_SOURCES 
    ${ORCA_ROOT}/src/imgui/*.cpp
    ${ORCA_ROOT}/src/imgui/backends/imgui_impl_opengl3.cpp
)

# 创建共享库
add_library(orcaslicer SHARED
    ${ANDROID_SOURCES}
    ${LIBSLIC3R_SOURCES}
    ${GUI_SOURCES}
    ${ADMESH_SOURCES}
    ${CLIPPER_SOURCES}
    ${POLYPARTITION_SOURCES}
    ${POLY2TRI_SOURCES}
    ${EXPAT_SOURCES}
    ${MINIZ_SOURCES}
    ${SEMVER_SOURCES}
    ${GLULIBTESS_SOURCES}
    ${IMGUI_SOURCES}
)

# 链接Android库
target_link_libraries(orcaslicer
    android
    log
    EGL
    GLESv3
    z
    jnigraphics
)

# 设置编译选项
target_compile_options(orcaslicer PRIVATE
    -Wall
    -Wno-sign-compare
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-unused-function
    -Wno-deprecated-declarations
    -Wno-missing-field-initializers
    -fexceptions
    -frtti
)

# 包含Android NDK目录
include_directories(SYSTEM
    ${ANDROID_NDK}/sources/android/native_app_glue
    ${ANDROID_NDK}/sources/android/cpufeatures
)