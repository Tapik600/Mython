cmake_minimum_required(VERSION 3.12)

project(mython_test LANGUAGES CXX)

aux_source_directory(. tests_src)

add_executable (${PROJECT_NAME} ${tests_src})
target_include_directories(${PROJECT_NAME} PRIVATE Mython_engine)
target_link_libraries(${PROJECT_NAME} Mython_engine)

add_test(${PROJECT_NAME} ${PROJECT_NAME})