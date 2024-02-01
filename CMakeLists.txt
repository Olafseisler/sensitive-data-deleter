cmake_minimum_required(VERSION 3.25)
project(sensitive-data-deleter)

include_directories(${Qt6Core_INCLUDE_DIRS} ${Qt6Gui_INCLUDE_DIRS} ${Qt6Widgets_INCLUDE_DIRS} ${Qt6Qml_INCLUDE_DIRS} ${Qt6Quick_INCLUDE_DIRS})

add_definitions(${Qt6Core_DEFINITIONS} ${Qt6Gui_DEFINITIONS} ${Qt6Widgets_DEFINITIONS} ${Qt6Qml_DEFINITIONS} ${Qt6Quick_DEFINITIONS})

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
        Qml
        Quick
        QuickControls2
        REQUIRED)

qt6_add_resources(QT_RESOURCES qml.qrc)

add_executable(${PROJECT} main.cpp ${QT_RESOURCES} treeitem.cpp treemodel.cpp)
target_link_libraries(${PROJECT}
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Qml
        Qt::Quick
        Qt::QuickControls2
        )

