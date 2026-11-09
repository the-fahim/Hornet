cmake_minimum_required(VERSION 3.14)

project(Hornet VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(hornet_engine
    src/lexer.cpp 
    src/parser.cpp 
    src/compiler.cpp 
    src/value.cpp 
    src/runtime.cpp
)