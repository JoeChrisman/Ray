cmake_minimum_required(VERSION 3.26)
project(Ray C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall")
set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall")

add_executable(Ray main.c
        Defs.h
        Defs.c
        Uci.h
        Uci.c
        Position.h
        Position.c
        Notation.c
        Notation.h
        Move.h
        AttackTables.h
        AttackTables.c
        MoveGen.h
        MoveGen.c
        Search.h
        Search.c
        Debug.h
        Debug.c
        Eval.h
        Eval.c
        Move.c
        HashTable.c
        HashTable.h
        MoveOrder.h
        MoveOrder.c
)
