cmake_minimum_required(VERSION 3.25)
project(sensitive-data-deleter)


#set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


set(PROJECT "SensitiveDataDeleter")

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)


add_executable(${PROJECT} src/main.cpp ${QT_RESOURCES} src/mainwindow.cpp src/mainwindow.h src/mainwindow.ui src/configmanager.cpp src/configmanager.h
        src/filescanner.cpp
        src/filescanner.h)
target_link_libraries(${PROJECT}
        Qt::Core
        Qt::Gui
        Qt::Widgets
        )


