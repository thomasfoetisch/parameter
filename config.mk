CXX = g++
DEPS_BIN = g++
DEPSFLAGS = -I/Users/thomashilke/.local/include
CXXFLAGS = -O3 -std=c++11 -I/Users/thomashilke/.local/include
LDFLAGS = -O3 -L/Users/thomashilke/.local/lib/
LDLIB = -llexer
AR = ar
ARFLAGS = rc
MKDIR = mkdir
MKDIRFLAGS = -p

PREFIX = ~/.local/
BIN_DIR = bin/
INCLUDE_DIR = include/
LIB_DIR = lib/


SOURCES = src/main.cpp src/parameter.cpp src/enums.cpp

HEADERS = include/parameter/parameter.hpp

BIN = bin/main bin/enums


#bin/...: ...
bin/main: build/src/main.o build/src/parameter.o
bin/enums: build/src/enums.o build/src/parameter.o

LIB = lib/libparameter.a

lib/libparameter.a: build/src/parameter.o
