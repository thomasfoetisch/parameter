CXX = clang++
DEPS_BIN = g++
DEPSFLAGS = -I$(HOME)/.local/include
CXXFLAGS = -O2 -std=c++11 -I$(HOME)/.local/include
LDFLAGS = -O2 -L$(HOME)/.local/lib/
LDLIB = -llexer
AR = ar
ARFLAGS = rc
MKDIR = mkdir
MKDIRFLAGS = -p

PREFIX = ~/.local/
BIN_DIR = bin/
INCLUDE_DIR = include/
LIB_DIR = lib/

PKG_NAME = parameter

SOURCES = src/main.cpp src/parameter.cpp src/enums.cpp src/collection.cpp

HEADERS = include/parameter/parameter.hpp

BIN = bin/main bin/enums bin/collection


#bin/...: ...
bin/main: build/src/main.o build/src/parameter.o
bin/enums: build/src/enums.o build/src/parameter.o
bin/collection: build/src/collection.o build/src/parameter.o

LIB = lib/libparameter.a

lib/libparameter.a: build/src/parameter.o
