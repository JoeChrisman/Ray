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
    NUMGAMES="1000"
fi

if [ -n "$4" ]; then
    TIMECTRL="$4"
else
    TIMECTRL="1+0.1"
fi

rm -f debug-pgns.log
rm -f debug-io.log

cutechess-cli \
    -debug all \
    -engine cmd="$ENGINE1" name="$NAME1" proto=uci restart=on \
    -engine cmd="$ENGINE2" name="$NAME2" proto=uci restart=on \
    -each tc="$TIMECTRL" \
    -games "$NUMGAMES" \
    -concurrency 8 \
    -recover \
    -pgnout debug-pgns.log \
    2>&1 | tee debug-io.log | grep --line-buffered "Started"


