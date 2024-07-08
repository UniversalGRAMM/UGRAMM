#######################################################
###########	Makefile for GRAMM          ###########
#######################################################

CXX = g++
CXXFLAGS = -I$(LIB_DIR) -lboost_graph

# Directories
SRC_DIR = src
LIB_DIR = lib
BUILD_DIR = build

EXE = GRAMM

# Target for the executable
$(EXE): $(SRC_DIR)/GRAMM.cpp $(LIB_DIR)/GRAMM.h
	$(CXX) $(SRC_DIR)/GRAMM.cpp $(CXXFLAGS) -o $(EXE) 

# Clean target to remove generated files
clean:
	rm -f $(EXE) *.dot *.txt *.png

