cmake_minimum_required(VERSION 3.28)
project(persistence CXX)
set(CMAKE_CXX_STANDARD 23)

# Collect the source files
file(GLOB_RECURSE DEMO_FILES "src/*.cpp")
add_executable(persistence ${DEMO_FILES})

# Set the path to the magique directory - inside this project we can just use the relative path
add_subdirectory(../../.. magique)

# Don't forget to link against magique
target_link_libraries(persistence PRIVATE magique)

# OPTIONAL - Add src include path for clean includes
target_include_directories(persistence PRIVATE src)