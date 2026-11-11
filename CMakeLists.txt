cmake_minimum_required(VERSION 3.16)
project(VisionDemo VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(VISION_OPENCV_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/Vision/OpenCV"
    CACHE  #缓存变量，gui界面用户可以改
    PATH    #PATH 表示这是一个目录路径，在 CMake GUI 里会显示为带“浏览…”按钮
    "OpenCV root的根目录" #文档字符串，提示这个变量
)

set(VISION_IMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/images")

option(VISION_USE_BUNDLED_OPENCV "使用项目自带的 MSVC x64 OpenCV, 而不是通过 find_package 查找 of find_package" ON)

if(VISION_USE_BUNDLED_OPENCV)
    if(NOT MSVC)
        message(FATAL_ERROR
            "Bundled OpenCV 3.4.16 is built for MSVC x64; the selected compiler is ${CMAKE_CXX_COMPILER_ID}: ${CMAKE_CXX_COMPILER}. "
            "Select an MSVC x64 Qt kit and a NEW build directory. "
            "For MinGW, supply a compatible MinGW OpenCV build with -DVISION_USE_BUNDLED_OPENCV=OFF -DOpenCV_DIR=<directory-containing-OpenCVConfig.cmake>.")
    endif()

    foreach(_file
        include/opencv2/core.hpp
        lib/opencv_world3416.lib
        lib/opencv_world3416d.lib
        opencv_world3416.dll
        opencv_world3416d.dll
    )
        if(NOT EXISTS "${VISION_OPENCV_ROOT}/${_file}")
            message(FATAL_ERROR "缺失OpenCV文件: ${VISION_OPENCV_ROOT}/${_file}")
        endif()
    endforeach()

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "编译的 OpenCV 必须要 x64 MSVC build...")
    endif()

    add_library(OpenCV SHARED IMPORTED)
    set_target_properties(OpenCV PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${VISION_OPENCV_ROOT}/include"
        IMPORTED_IMPLIB "${VISION_OPENCV_ROOT}/lib/opencv_world3416.lib"
        IMPORTED_LOCATION "${VISION_OPENCV_ROOT}/opencv_world3416.dll"
        IMPORTED_IMPLIB_DEBUG "${VISION_OPENCV_ROOT}/lib/opencv_world3416d.lib"
        IMPORTED_LOCATION_DEBUG "${VISION_OPENCV_ROOT}/opencv_world3416d.dll")
else()
    #使用查找OpenCV包的形式
    find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
    add_library(vision_opencv INTERFACE)
    target_include_directories(vision_opencv INTERFACE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(vision_opencv INTERFACE ${OpenCV_LIBS})
endif()
add_library(OpenCV::OpenCV ALIAS OpenCV)


#
add_library(circle_detection
    detection/CircleTypes.h
    detection/CircleFitter.h
    detection/CircleFitter.cpp
    detection/CircleDetector.h 
    detection/CircleDetector.cpp
)
add_library(circle_detection::circle_detection ALIAS circle_detection)

target_include_directories(circle_detection PUBLIC 
"${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(circle_detection PUBLIC 
OpenCV::OpenCV
)

if(MSVC)
    # 告诉编译器：源文件按 UTF-8 解析，使用第四级报警提示
    add_compile_options(/utf-8 /W4)
endif()



# ================ 复制OPenCV库函数 ================================
# 定义一个函数，传入目标名称(dll,exe)
function(Copy_OpenCV_Lib target)
    if(VISION_USE_BUNDLED_OPENCV)
        add_custom_command(TARGET ${target}
            POST_BUILD COMMAND # 目标链接成功后执行
            ${CMAKE_COMMAND} -E copy_if_different # 用 CMake 自带的可移植命令 copy_if_different 做复制
            "$<TARGET_FILE:OpenCV>" # OpenCV依赖库
            "$<TARGET_FILE_DIR:${target}>") # EXE所在的路径
    endif()
endfunction()

# ========================查找Qt库==================================
if(MSVC)
    set(QT_DIR "D:/Qt/Qt5.12.12/5.12.12/msvc2017_64")
elseif(MINGW AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(QT_DIR "D:/Qt/Qt5.12.12/5.12.12/mingw73_64")
elseif(MINGW)
    set(QT_DIR "D:/Qt/Qt5.12.12/5.12.12/mingw73_32")
endif()

set(Qt_COMPONENT_List LinguistTools Core Widgets Gui AxContainer Sql Xml Network SerialPort SerialBus)
find_package(QT NAMES Qt6 Qt5 COMPONENTS ${Qt_COMPONENT_List} REQUIRED HINTS "${QT_DIR}")
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS ${Qt_COMPONENT_List} REQUIRED HINTS "${QT_DIR}")

if(Qt5Widgets_FOUND OR Qt6Widgets_FOUND)
    message("QT_DIR : ${QT_DIR}")
endif()

# ============================可执行文件的编译==========================
add_executable(VisionDemo WIN32
    main.cpp
    io/ImageDocument.h
    io/ImageDocument.cpp
    ImageView.h
    ImageView.cpp
    MainWindow.h
    MainWindow.cpp
    MainWindow.ui
)

set_target_properties(VisionDemo PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTOMOC ON
)
target_link_libraries(VisionDemo PRIVATE
    Qt5::Widgets
    Qt5::Core
    circle_detection::circle_detection
)

Copy_OpenCV_Lib(VisionDemo)

add_custom_command(TARGET VisionDemo
    POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_directory #拷贝整个图片文件夹
    "${VISION_IMAGE_DIR}" #图片文件夹路径
    "$<TARGET_FILE_DIR:VisionDemo>/images"  #拷贝到VisionDemo这个可执行文件生成的路径/images下
)

# # ==============获取qmake所在的路径，用windeployqt来打包=============
option(WindeployQt ON)

if(WindeployQt)
    get_target_property(qt_qmake Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(qt_bin "${qt_qmake}" DIRECTORY)

    # 部署工具必须来自链接时使用的 Qt，不能沿用旧套件中的工具。
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${qt_bin}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET VisionDemo POST_BUILD COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:VisionDemo>")
            
    endif()
endif()
