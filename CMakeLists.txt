cmake_minimum_required(VERSION 3.25)
project(sensitive-data-deleter)

set(VCPKG_TARGET_TRIPLET "x64-windows-static")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


set(PROJECT "SensitiveDataDeleter")

find_package(Qt6 6.7.2 REQUIRED COMPONENTS
        Core
        Gui
        Widgets
        )

find_package(PkgConfig)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include)

add_executable(${PROJECT} src/main.cpp ${QT_RESOURCES}
        src/mainwindow.cpp
        src/mainwindow.h
        src/mainwindow.ui
        src/configmanager.cpp
        src/configmanager.h
        src/filescanner.cpp
        src/filescanner.h
        src/chunkreader.cpp)

target_link_libraries(${PROJECT}
        Qt::Core
        Qt::Gui
        Qt::Widgets
        PkgConfig::POPPLER_CPP
        ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib/hs.lib
)


