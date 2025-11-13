CC=gcc
C-FLAGS=-O3
CPP=g++
CPP-FLAGS=-std=c++14 -O3

all: vm vm-alt

vm-alt: vm-alt.cpp
	${CPP} ${CPP-FLAGS} $^ -o $@

vm: vm.c
	${CC} ${C-FLAGS} $^ -o $@

.PHONY:
clean:
	rm -f vm
	rm -f vm-alt