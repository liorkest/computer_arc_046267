# 046267 Computer Architecture - HW #1
# makefile for test environment

all: bp_main

# Environment for C 
CC = gcc
CFLAGS = -std=c99 -Wall

# Environment for C++ 
CXX = g++
CXXFLAGS = -std=c++11 -Wall

# Automatically detect whether the bp is C or C++
# Must have either bp.c or bp.cpp - NOT both
SRC_BP = $(wildcard \CompArch-hw1\bp.c \CompArch-hw1\bp.cpp)
SRC_GIVEN = .\CompArch-hw1\bp_main.c
EXTRA_DEPS = .\CompArch-hw1\bp_api.h

OBJ_GIVEN = $(patsubst %.c,%.o,$(SRC_GIVEN))
OBJ_BP = .\CompArch-hw1\bp.o
OBJ = $(OBJ_GIVEN) $(OBJ_BP)

#$(info OBJ=$(OBJ))


ifeq ($(SRC_BP),.\CompArch-hw1\bp.c)
.\CompArch-hw1\bp_main: $(OBJ)
	$(CC)  -o $@ $(OBJ) -lm

.\CompArch-hw1\bp.o: .\CompArch-hw1\bp.c
	$(CC) -c $(CFLAGS)  -o $@ $^ -lm

else
.\CompArch-hw1\bp_main: $(OBJ)
	$(CXX) -o $@ $(OBJ)

.\CompArch-hw1\bp.o: .\CompArch-hw1\bp.cpp
	$(CXX) -c $(CXXFLAGS)  -o $@ $^ -lm
endif

$(OBJ_GIVEN): %.o: %.c
	$(CC) -c $(CFLAGS)  -o $@ $^ -lm


.PHONY: clean
clean:
	rm -f .\CompArch-hw1\bp_main $(OBJ)
