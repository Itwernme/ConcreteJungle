#! /bin/bash
set -e
cc -g -std=c99 -c main.c -o obj/main.o
cc -g -std=c99 -c game.c -o obj/game.o
cc -o build/main obj/main.o obj/game.o -s -Wall -std=c99 -lraylib -lm -lpthread -ldl -lrt
./build/main
