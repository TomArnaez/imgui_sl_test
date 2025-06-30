cmake_minimum_required(VERSION 3.30.0)
project(vulkan_engine VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

add_subdirectory(app)
add_subdirectory(lib)