#######################################################
###########   Makefile for GRAMM          ###########
#######################################################

BOOST_INCLUDE = $(shell whereis boost | awk '{print $$2 "/include"}')
BOOST_LIB = $(shell whereis boost | awk '{print $$2 "/lib"}')

CXX = g++
CXXFLAGS = -I$(LIB_DIR) -g -I$(BOOST_INCLUDE)

LDFLAGS = -L$(BOOST_LIB) -lfmt -lboost_graph -lboost_program_options

# Directories
SRC_DIR = src
LIB_DIR = lib
BUILD_DIR = build

EXE = UGRAMM

# Source files
SRCS = $(SRC_DIR)/UGRAMM.cpp $(SRC_DIR)/routing.cpp $(SRC_DIR)/utilities.cpp $(SRC_DIR)/drc.cpp

# Object files
OBJS = $(BUILD_DIR)/UGRAMM.o $(BUILD_DIR)/routing.o $(BUILD_DIR)/utilities.o $(BUILD_DIR)/drc.o

# Target for the executable
$(EXE): $(OBJS)
	$(CXX) $(OBJS) $(CXXFLAGS) -o $(EXE) $(LDFLAGS)

# Rule to build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(LIB_DIR)/%.h
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target to remove generated files
clean:
	rm -f $(EXE) $(BUILD_DIR)/*.o *.dot *.txt *.png
