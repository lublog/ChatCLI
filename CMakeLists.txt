cmake_minimum_required(VERSION 3.25)
project(Main C)

set(CMAKE_C_STANDARD 11)

add_executable(Main main.c word_art.h)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lncurses -lpanel")