cmake_minimum_required(VERSION 3.4)
project(Clip-8 C)

set(CMAKE_C_STANDARD 11)

include(FetchContent)

# Raylib master
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG master
)
FetchContent_MakeAvailable(raylib)


add_executable(Clip-8 src/clip-8.c)


target_link_libraries(Clip-8 PRIVATE raylib)
