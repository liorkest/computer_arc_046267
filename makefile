# 046267 Computer Architecture - HW #1
# makefile for test environment

all: bp_main

# Environment for C 
CC = gcc
CFLAGS = -std=c99 -Wall -g

# Environment for C++ 
CXX = g++
CXXFLAGS = -std=c++11 -Wall -g

# Automatically detect whether the bp is C or C++
# Must have either bp.c or bp.cpp - NOT both
SRC_BP = $(wildcard bp.c bp.cpp)
SRC_GIVEN = bp_main.c
EXTRA_DEPS = bp_api.h

OBJ_GIVEN = $(patsubst %.c,%.o,$(SRC_GIVEN))
OBJ_BP = bp.o
OBJ = $(OBJ_GIVEN) $(OBJ_BP)

# Determine whether we're compiling C or C++ based on the presence of bp.c or bp.cpp
ifeq ($(SRC_BP),bp.c)
bp_main: $(OBJ)
	$(CC) -o $@ $(OBJ) -lm

bp.o: bp.c
	$(CC) -c $(CFLAGS) -o $@ $^ -lm

else
bp_main: $(OBJ)
	$(CXX) -o $@ $(OBJ)

bp.o: bp.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $^ -lm
endif

$(OBJ_GIVEN): %.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^ -lm

.PHONY: clean valgrind
clean:
	rm -f bp_main $(OBJ)

valgrind: bp_main
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./bp_main
