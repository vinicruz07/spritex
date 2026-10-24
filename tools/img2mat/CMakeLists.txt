cmake_minimum_required(VERSION 3.16)
project(img2mat VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../mat2png)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(img2mat main.cpp)

set_target_properties(img2mat PROPERTIES
    OUTPUT_NAME "img2mat"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
