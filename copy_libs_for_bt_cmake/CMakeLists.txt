# Copying Boost libraries for a CMake project
# need to remove fmt in conda: conda remove fmt
cmake_minimum_required(VERSION 3.14)
project(copyLibs_1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS regex)

add_executable(copyLibs_1 src/main.cpp)

find_package(fmt CONFIG REQUIRED)

target_include_directories(copyLibs_1 PRIVATE
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(copyLibs_1
    PRIVATE
    ${Boost_LIBRARIES}
    fmt::fmt
    pthread
    dl
)