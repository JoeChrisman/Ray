cmake_minimum_required(VERSION 3.26)
project(Ray C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall")
set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall")

add_executable(Ray main.c
        Uci.h
        Uci.c
        Position.h
        Position.c
        Notation.c
        Notation.h
        Move.h
        Move.c
        AttackTables.h
        AttackTables.c
        MoveGen.h
        MoveGen.c
        Search.h
        Search.c
        Eval.h
        Eval.c
        HashTable.c
        HashTable.h
        MoveOrder.h
        MoveOrder.c
        Bitboard.h
        Bitboard.c
        Utils.h
        Utils.c
        Perft.h
        Perft.c
        Piece.h
        Square.h
        SearchManager.h
        SearchManager.c
)
