cmake_minimum_required(VERSION 3.12)

project(
    Mython_engine
    VERSION 1.0
    LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 17)

set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror -Wno-unused-parameter -Wno-implicit-fallthrough"
  )

file(GLOB SOURCE "src/*.cpp")

add_library(${PROJECT_NAME} ${SOURCE})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
)

add_subdirectory(app)

option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()