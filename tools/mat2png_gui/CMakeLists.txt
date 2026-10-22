cmake_minimum_required(VERSION 3.16)
project(mat2png_gui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../mat2png)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mat2png_gui WIN32 main.cpp)

target_link_libraries(mat2png_gui PRIVATE
    gdi32
    user32
    comdlg32
    comctl32
)

set_target_properties(mat2png_gui PROPERTIES
    OUTPUT_NAME "mat2png_gui"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
