cmake_minimum_required(VERSION 3.26)
project(Ray C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

add_executable(Ray main.c
        Defs.h
        Uci.h
        Uci.c
        Position.h
        Position.c
        Notation.c
        Notation.h
        Move.h
        Magics.h
        Magics.c
        Defs.c
)
