cmake_minimum_required(VERSION 3.25)
project(sensitive-data-deleter)

include_directories(${Qt6Core_INCLUDE_DIRS} ${Qt6Gui_INCLUDE_DIRS} ${Qt6Widgets_INCLUDE_DIRS} )

add_definitions(${Qt6Core_DEFINITIONS} ${Qt6Gui_DEFINITIONS} ${Qt6Widgets_DEFINITIONS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# build statically linked binary


set(CMAKE_PREFIX_PATH "${Qt6_DIR}")
set(PROJECT "SensitiveDataDeleter")

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)


add_executable(${PROJECT} main.cpp ${QT_RESOURCES} mainwindow.cpp mainwindow.h mainwindow.ui)
target_link_libraries(${PROJECT}
        Qt::Core
        Qt::Gui
        Qt::Widgets
        )

