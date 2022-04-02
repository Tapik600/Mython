cmake_minimum_required(VERSION 3.12)

project(
    Mython
    VERSION 1.0.0
    LANGUAGES CXX
)
configure_file(config.h.in config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable (${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE Mython_engine ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${PROJECT_NAME} Mython_engine)
