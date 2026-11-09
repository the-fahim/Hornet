cmake_minimum_required(VERSION 3.14)

project(Hornet VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(hornet_engine
    src/lexer.cpp 
    src/parser.cpp 
    src/compiler.cpp 
    src/value.cpp 
    src/runtime.cpp
)

# Use PUBLIC so any executable linking to hornet_engine also gets the include path
target_include_directories(hornet_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Define your main executable and link it to the engine
add_executable(output src/main.cpp)
target_link_libraries(output PRIVATE hornet_engine)