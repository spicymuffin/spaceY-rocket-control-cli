cmake_minimum_required(VERSION 3.12)

set(PROJECT RCCLI)

project(${PROJECT} VERSION 1.0 DESCRIPTION "SpaceY rocket control cli" LANGUAGES C CXX ASM)

#
# Set Language Standards
#
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(${PROJECT}_main ${SOURCES})

target_link_libraries(${PROJECT}_main PRIVATE)
target_link_options(${PROJECT}_main PRIVATE -Xlinker --print-memory-usage)

target_include_directories(${PROJECT}_main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)