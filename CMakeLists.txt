cmake_minimum_required(VERSION 3.12)
project(hellocmake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(main main.cpp)

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
target_link_libraries(main PUBLIC Threads::Threads)
target_link_libraries(main PUBLIC OpenMP::OpenMP_CXX)
target_compile_options(main PUBLIC -O3  -ffast-math -march=native)
