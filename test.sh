#!/bin/bash

if [ -n "$1" ]; then
    ENGINE1="$1"
else
    ENGINE1="./Ray"
fi
NAME1=$(basename "$ENGINE1")

if [ -n "$2" ]; then
    ENGINE2="$2"
else
    ENGINE2="./Ray"
fi
NAME2=$(basename "$ENGINE2")

if [ -n "$3" ]; then
    NUMGAMES="$3"
else
    NUMGAMES="6000"
fi

if [ -n "$4" ]; then
    TIMECTRL="$4"
else
    TIMECTRL="1+0.1"
fi

cutechess-cli \
    -engine cmd="$ENGINE1" name="$NAME1" proto=uci \
    -engine cmd="$ENGINE2" name="$NAME2" proto=uci \
    -each tc="$TIMECTRL" \
    -games "$NUMGAMES" \
    -concurrency 8 \
    -recover


