cmake_minimum_required(VERSION 3.14)

project(Hornet LANGUAGES CXX)

# Enable ccache if available
find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
    output
    src/main.cpp
)