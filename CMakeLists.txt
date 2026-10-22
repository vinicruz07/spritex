cmake_minimum_required(VERSION 3.16)
project(spritex VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)
include_directories(third_party)

# Source files
file(GLOB_RECURSE SPRITEX_SOURCES "src/*.cpp")

# Executable target
add_executable(spritex ${SPRITEX_SOURCES})

# Target properties
set_target_properties(spritex PROPERTIES
    OUTPUT_NAME "spritex"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Subdirectories
add_subdirectory(tools/mat2png)
add_subdirectory(tools/mat2png_gui)

