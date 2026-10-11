cmake_minimum_required(VERSION 3.14)
project(CompilerOptimizationVisualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Main compiler executable
add_executable(compiler
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/ir_generator.cpp
    src/constant_folding.cpp
    src/constant_propagation.cpp
    src/dead_code_elimination.cpp
    src/interpreter.cpp
    src/json_output.cpp
)

target_include_directories(compiler PRIVATE include)

# Treat warnings as informational, not errors
if(MSVC)
    target_compile_options(compiler PRIVATE /W4)
else()
    target_compile_options(compiler PRIVATE -Wall -Wextra -Wpedantic)
endif()
