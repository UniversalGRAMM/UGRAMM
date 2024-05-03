#######################################################
###########	Makefile for GRAMM          ###########
#######################################################

CXX = g++
CXXFLAGS = -lboost_graph

# Directories
SRC_DIR = src
LIB_DIR = lib
BUILD_DIR = build

EXE = GRAMM

$(EXE): $(SRC_DIR)/GRAMM.cpp
	g++ $(SRC_DIR)/GRAMM.cpp $(CXXFLAGS) -o $(EXE) 

clean:
	rm $(EXE) *.dot *.txt *.png


