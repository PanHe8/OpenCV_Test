cmake_minimum_required(VERSION 3.16)
project(VisionDemo VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VISION_BUILD_GUI "Build the Qt Widgets application" ON)
set(VISION_OPENCV_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Vision/OpenCV" CACHE PATH "Bundled OpenCV root")
set(VISION_IMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/images" CACHE PATH "Sample image directory (readable by the application)")
set(_vision_bundled_default OFF)
if(WIN32 AND EXISTS "${VISION_OPENCV_ROOT}/lib/opencv_world3416.lib")
    set(_vision_bundled_default ON)
endif()
option(VISION_USE_BUNDLED_OPENCV "Use the bundled MSVC x64 OpenCV instead of find_package" ${_vision_bundled_default})
if(VISION_USE_BUNDLED_OPENCV)
    if(NOT MSVC)
        message(FATAL_ERROR
            "Bundled OpenCV 3.4.16 is built for MSVC x64; the selected compiler is ${CMAKE_CXX_COMPILER_ID}: ${CMAKE_CXX_COMPILER}. "
            "Select an MSVC x64 Qt kit and a NEW build directory. "
            "For MinGW, supply a compatible MinGW OpenCV build with -DVISION_USE_BUNDLED_OPENCV=OFF -DOpenCV_DIR=<directory-containing-OpenCVConfig.cmake>.")
    endif()
    foreach(_file include/opencv2/core.hpp lib/opencv_world3416.lib lib/opencv_world3416d.lib opencv_world3416.dll opencv_world3416d.dll)
        if(NOT EXISTS "${VISION_OPENCV_ROOT}/${_file}")
            message(FATAL_ERROR "Missing bundled OpenCV file: ${VISION_OPENCV_ROOT}/${_file}")
        endif()
    endforeach()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Bundled OpenCV requires an x64 MSVC build. Use -A x64.")
    endif()
    add_library(vision_opencv SHARED IMPORTED)
    set_target_properties(vision_opencv PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${VISION_OPENCV_ROOT}/include"
        IMPORTED_IMPLIB "${VISION_OPENCV_ROOT}/lib/opencv_world3416.lib"
        IMPORTED_LOCATION "${VISION_OPENCV_ROOT}/opencv_world3416.dll"
        IMPORTED_IMPLIB_DEBUG "${VISION_OPENCV_ROOT}/lib/opencv_world3416d.lib"
        IMPORTED_LOCATION_DEBUG "${VISION_OPENCV_ROOT}/opencv_world3416d.dll")
else()
    # Respect an externally supplied OpenCV_DIR. The bundled package's generated
    # config refers to its original install layout and must not be used here.
    find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
    add_library(vision_opencv INTERFACE)
    target_include_directories(vision_opencv INTERFACE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(vision_opencv INTERFACE ${OpenCV_LIBS})
endif()

add_library(circle_detection
    detection/CircleTypes.h
    detection/CircleFitter.h detection/CircleFitter.cpp
    detection/CircleDetector.h detection/CircleDetector.cpp)
target_include_directories(circle_detection PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(circle_detection PUBLIC vision_opencv)
if(MSVC)
    add_compile_options(/utf-8 /W4)
    target_compile_options(circle_detection PRIVATE /utf-8 /W4)
endif()

function(vision_copy_opencv target)
    if(VISION_USE_BUNDLED_OPENCV)
        add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:vision_opencv>" "$<TARGET_FILE_DIR:${target}>")
    endif()
endfunction()

if(VISION_BUILD_GUI)

    if(MSVC)
        set(QT_DIR "D:/Qt/Qt5.12.12/5.12.12/msvc2017_64/lib/cmake")
    else()
        set(QT_DIR "D:/Qt/Qt5.12.12/5.12.12/mingw73_32/lib/cmake")
    endif()
    find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets)
    add_executable(VisionDemo WIN32 main.cpp
        io/ImageDocument.h io/ImageDocument.cpp
        ImageView.h ImageView.cpp
        MainWindow.h MainWindow.cpp MainWindow.ui)
    set_target_properties(VisionDemo PROPERTIES AUTOMOC ON AUTOUIC ON)
    target_link_libraries(VisionDemo PRIVATE circle_detection Qt5::Widgets)
    vision_copy_opencv(VisionDemo)
    add_custom_command(TARGET VisionDemo POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${VISION_IMAGE_DIR}" "$<TARGET_FILE_DIR:VisionDemo>/images")
    get_target_property(_qt_qmake Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin "${_qt_qmake}" DIRECTORY)
    find_program(VISION_WINDEPLOYQT windeployqt HINTS "${_qt_bin}")
    if(WIN32 AND VISION_WINDEPLOYQT)
        add_custom_command(TARGET VisionDemo POST_BUILD COMMAND ${CMAKE_COMMAND} -E env
            "PATH=${_qt_bin};$ENV{SystemRoot}/System32;$ENV{SystemRoot}" "${VISION_WINDEPLOYQT}"
            --force "$<IF:$<CONFIG:Debug>,--debug,--release>" --no-compiler-runtime
            --no-translations --no-system-d3d-compiler --no-opengl-sw "$<TARGET_FILE:VisionDemo>"
            VERBATIM)
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts")
    add_executable(circle_tests tests/CircleTests.cpp)
    target_link_libraries(circle_tests PRIVATE circle_detection)
    vision_copy_opencv(circle_tests)
    add_test(NAME circle_geometry COMMAND circle_tests --synthetic)
    add_test(NAME circle_regression COMMAND circle_tests
        "${VISION_IMAGE_DIR}/FPC1_mountPickTrain.bmp"
        "${VISION_IMAGE_DIR}/FPC1_mountPickTrain_Origin.bmp"
        "${CMAKE_CURRENT_BINARY_DIR}/test-artifacts")
    if(VISION_BUILD_GUI)
        add_executable(vision_gui_tests tests/GuiTests.cpp
            io/ImageDocument.h io/ImageDocument.cpp
            ImageView.h ImageView.cpp MainWindow.h MainWindow.cpp MainWindow.ui)
        set_target_properties(vision_gui_tests PROPERTIES AUTOMOC ON AUTOUIC ON)
        target_link_libraries(vision_gui_tests PRIVATE circle_detection Qt5::Widgets)
        # 复用主程序部署到同一输出目录的 Qt 和 OpenCV 运行库。
        add_dependencies(vision_gui_tests VisionDemo)
        add_test(NAME gui_interaction COMMAND vision_gui_tests
            "${VISION_IMAGE_DIR}/FPC1_mountPickTrain.bmp"
            "${VISION_IMAGE_DIR}/FPC1_mountPickTrain_Origin.bmp")
    endif()
endif()
