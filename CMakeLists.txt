cmake_minimum_required(VERSION 3.16)
project(DailyRoutineDashboard VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic compilation of Qt Meta-Object System files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

add_executable(DailyRoutineDashboard
    main.cpp
    storage.h
    storage.cpp
    logic.h
    logic.cpp
    ui.h
    ui.cpp
    analytics.h
    analytics.cpp
)

target_link_libraries(DailyRoutineDashboard PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Platform specific configurations
if(WIN32)
    # Hide terminal console window in release mode
    set_target_properties(DailyRoutineDashboard PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
