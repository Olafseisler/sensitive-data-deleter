cmake_minimum_required(VERSION 3.25)
project(sensitive-data-deleter)

if (WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

#set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


set(PROJECT "SensitiveDataDeleter")

find_package(Qt6 6.7.2 COMPONENTS Core Widgets Gui REQUIRED)
find_package(minizip-ng REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)

include_directories(${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows-static/include/hs)


add_executable(${PROJECT} src/main.cpp ${QT_RESOURCES}
        src/mainwindow.cpp
        src/mainwindow.h
        src/mainwindow.ui
        src/configmanager.cpp
        src/configmanager.h
        src/filescanner.cpp
        src/filescanner.h
        src/chunkreader.cpp
        src/chunkreader.h)

target_link_libraries(${PROJECT}
        Qt::Core
        Qt::Gui
        Qt::Widgets
        MINIZIP::minizip-ng
        tinyxml2::tinyxml2
        PkgConfig::POPPLER_CPP

        ${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows-static/lib/hs.lib
        )


