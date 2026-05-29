cmake_minimum_required(VERSION 3.16)

project(LvglFontGenerator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 为MSVC添加C++17标志
if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# 使用 Qt 自带的 FreeType 库
set(FREETYPE_INCLUDE_DIRS "${Qt6_DIR}/../../../include/QtFreetype")

# 根据编译器选择正确的库文件扩展名
if(MSVC)
    set(FREETYPE_LIBRARY "${Qt6_DIR}/../../../lib/Qt6BundledFreetype.lib")
    set(LIBPNG_LIBRARY "${Qt6_DIR}/../../../lib/Qt6BundledLibpng.lib")
else()
    set(FREETYPE_LIBRARY "${Qt6_DIR}/../../../lib/libQt6BundledFreetype.a")
    set(LIBPNG_LIBRARY "${Qt6_DIR}/../../../lib/libQt6BundledLibpng.a")
endif()

if(EXISTS "${FREETYPE_LIBRARY}")
    set(FREETYPE_LIBRARIES "${FREETYPE_LIBRARY}" "${LIBPNG_LIBRARY}")
    message(STATUS "Using Qt bundled FreeType: ${FREETYPE_LIBRARY}")
    message(STATUS "Using Qt bundled LibPNG: ${LIBPNG_LIBRARY}")
    message(STATUS "FreeType include: ${FREETYPE_INCLUDE_DIRS}")
else()
    # 如果找不到Qt自带的freetype，尝试系统freetype
    find_package(Freetype REQUIRED)
    if(FREETYPE_FOUND)
        message(STATUS "FreeType found: ${FREETYPE_LIBRARIES}")
        message(STATUS "FreeType include: ${FREETYPE_INCLUDE_DIRS}")
    endif()
endif()

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/fontgenerator.cpp
    src/fontgenerator.h
    src/lvglexporter.cpp
    src/lvglexporter.h
    src/fontpreviewwidget.cpp
    src/fontpreviewwidget.h
    src/characterinputwidget.cpp
    src/characterinputwidget.h
    src/freetyperenderer.cpp
    src/freetyperenderer.h
    src/kerningoptimizer.cpp
    src/kerningoptimizer.h
)

if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES})
else()
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${FREETYPE_LIBRARIES}
)

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
