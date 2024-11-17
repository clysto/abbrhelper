cmake_minimum_required(VERSION 3.15)

project(abbrhelper VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

include(FetchContent)
FetchContent_Declare(rapidfuzz
    GIT_REPOSITORY https://github.com/rapidfuzz/rapidfuzz-cpp.git
    GIT_TAG main
)
FetchContent_MakeAvailable(rapidfuzz)

qt_standard_project_setup()

qt_add_executable(abbrhelper
    src/main.cpp
    src/mainwindow.hpp
    src/mainwindow.cpp
    src/utils.hpp
    src/utils.cpp
    src/resources.qrc
)

target_link_libraries(abbrhelper PRIVATE Qt6::Widgets Qt6::Concurrent rapidfuzz::rapidfuzz)

set_target_properties(abbrhelper PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
