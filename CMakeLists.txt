cmake_minimum_required(VERSION 3.25)
project(sensitive-data-deleter)


#set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_PREFIX_PATH "G:\\Qt\\6.7.1\\mingw_64")


set(PROJECT "SensitiveDataDeleter")

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)


add_executable(${PROJECT} main.cpp ${QT_RESOURCES} mainwindow.cpp mainwindow.h mainwindow.ui configmanager.cpp configmanager.h
        filescanner.cpp
        filescanner.h)
target_link_libraries(${PROJECT}
        Qt::Core
        Qt::Gui
        Qt::Widgets
        )


