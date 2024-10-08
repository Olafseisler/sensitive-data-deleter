cmake_minimum_required(VERSION 3.25)
project(sensitive-data-deleter)

if (WIN32)
        # If is a debug build
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        else ()
                set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
                set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        endif()
        set(VCPKG_CRT_LINKAGE static)
        set(VCPKG_LIBRARY_LINKAGE static)
        include_directories(${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows-static/include/hs)
elseif (APPLE)
        include_directories(/opt/homebrew/Cellar/vectorscan/5.4.11/include)
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

if (WIN32)
        add_executable(${PROJECT} 
                src/main.cpp ${QT_RESOURCES}
                src/mainwindow.cpp
                src/mainwindow.h
                src/mainwindow.ui
                src/configmanager.cpp
                src/configmanager.h
                src/filescanner.cpp
                src/filescanner.h
                src/chunkreader.cpp
                src/chunkreader.h)

        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
                target_link_libraries(${PROJECT}
                        Qt::Core
                        Qt::Gui
                        Qt::Widgets
                        PkgConfig::POPPLER_CPP
                        MINIZIP::minizip-ng
                        tinyxml2::tinyxml2
                        ${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows-static/debug/lib/hs.lib
                )
        else()
                target_link_libraries(${PROJECT}
                        Qt::Core
                        Qt::Gui
                        Qt::Widgets
                        PkgConfig::POPPLER_CPP
                        MINIZIP::minizip-ng
                        tinyxml2::tinyxml2
                        ${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows-static/lib/hs.lib
                )
        endif()
elseif(APPLE)
        add_executable(${PROJECT}
                src/main.cpp ${QT_RESOURCES}
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
                PkgConfig::POPPLER_CPP
                minizip-ng::minizip-ng
                tinyxml2::tinyxml2
                /opt/homebrew/Cellar/vectorscan/5.4.11/lib/libhs.a
        )
endif()





