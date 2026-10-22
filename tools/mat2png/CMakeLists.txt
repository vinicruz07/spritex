cmake_minimum_required(VERSION 3.16)
project(mat2png VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mat2png main.cpp)

set_target_properties(mat2png PROPERTIES
    OUTPUT_NAME "mat2png"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
