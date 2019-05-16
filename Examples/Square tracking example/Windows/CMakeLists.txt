#
# artoolkitX Examples CMake configuration file.
# Square tracking example
#
# To invoke CMake on this file, from the chosen build directory, invoke:
#     cmake <path to directory containing this file>.
#
# Author(s): Philip Lamb <phil@artoolkitx.org>
#

cmake_minimum_required( VERSION 3.2 )

# Any directives that must go before the project command.

project("artoolkitx_square_tracking_example"
        VERSION 1.0.0
        LANGUAGES CXX C
)

set(ARTOOLKITX_HOME "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

add_definitions("-DARX_TARGET_PLATFORM_WINDOWS=1")

if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_definitions("-DDEBUG=1")
endif()

# Search prefix.
list(APPEND CMAKE_MODULE_PATH "${ARTOOLKITX_HOME}/Source/cmake")
if(ARX_TARGET_PLATFORM_VARIANT)
    list(APPEND CMAKE_FIND_ROOT_PATH "${ARTOOLKITX_HOME}/Source/depends/windows-${ARX_TARGET_PLATFORM_VARIANT}")
else()
    list(APPEND CMAKE_FIND_ROOT_PATH "${ARTOOLKITX_HOME}/Source/depends/windows")
endif()

# Installation settings.
# If the user didn't define CMAKE_INSTALL_PREFIX, set a local install destination.
# When building packages, this will typically be overridden with DESTDIR (e.g. "make install DESTDIR=/tmp").
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${ARTOOLKITX_HOME}/SDK" CACHE PATH "default install path" FORCE )
endif()

# Enable C++11 for GCC/Clang compilers.
set(CMAKE_CXX_STANDARD "11")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SDL2_PATH "${CMAKE_SOURCE_DIR}/../../../Source/depends/windows")
find_package(SDL2 REQUIRED)
string(STRIP "${SDL2_LIBRARY}" SDL2_LIBRARY)
include_directories(${SDL2_INCLUDE_DIR})

find_package(OpenGL)
find_package(OpenGLES2)
set(GL3_PATH "${ARTOOLKITX_HOME}/Source/depends/windows/include")
find_package(OpenGL3)
if ((OPENGL_FOUND OR OpenGL3_FOUND) AND OPENGLES2_FOUND)
    if(GL_PREFER_EMBEDDED)
        set(OPENGL_FOUND 0)
        set(OpenGL3_FOUND 0)
        message(STATUS "Opting for embedded OpenGL")
    else()
        set(OPENGLES2_FOUND 0)
        message(STATUS "Opting for desktop OpenGL. Set GL_PREFER_EMBEDDED to change.")
    endif()
endif()

include(${ARTOOLKITX_HOME}/SDK/lib/ARX/ARX.cmake)

set(OpenCV_PATH "${ARTOOLKITX_HOME}/Source/depends/windows")
find_package(OpenCV)
if(OpenCV_FOUND)
    set(HAVE_OPENCV 1)
    set(HAVE_2D 1)
else()
    message(WARNING "OpenCV not found, will build without 2D tracker" )
endif()


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

set(RESOURCES
    ../hiro.patt
    ../kanji.patt
)

set(SOURCE
    ../main.cpp
    ../draw.h
    ../draw.cpp
    ../../../Source/ARX/AR/icpCalibStereo.c
    ../../../Source/ARX/AR/icpCore.c
    ../../../Source/ARX/AR/icpHandle.c
    ../../../Source/ARX/AR/icpPoint.c
    ../../../Source/ARX/AR/icpUtil.c
    ${RESOURCES}
    ${FRAMEWORKS}
)

add_executable(${PROJECT_NAME} ${SOURCE})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

add_dependencies(${PROJECT_NAME}
    ARX
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "\$ORIGIN/../lib"
)

target_link_libraries(${PROJECT_NAME}
    ARX
    ${SDL2_LIBRARY}
    "${ARTOOLKITX_HOME}/Source/depends/windows/lib/x64/opencv_calib3d341d.lib"
    "${ARTOOLKITX_HOME}/Source/depends/windows/lib/x64/opencv_core341d.lib"
    "${ARTOOLKITX_HOME}/Source/depends/windows/lib/x64/opencv_features2d341d.lib"
    "${ARTOOLKITX_HOME}/Source/depends/windows/lib/x64/opencv_imgproc341d.lib"
)

if(OPENGL_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OPENGL_INCLUDE_DIR})
endif()
if(OPENGLES2_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OPENGLES2_INCLUDE_DIR})
endif()
if(OpenGL3_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OpenGL3_INCLUDE_PATH})
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(FILES ${RESOURCES} DESTINATION bin)