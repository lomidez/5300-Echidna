# Makefile, Kevin Lundeen, Seattle University, CPSC5300, Winter Quarter 2024
CXX      ?= g++
CPPFLAGS  = -I/usr/local/db6/include -I$(INC_DIR) #-Wall -Wextra -Wpedantic
CXXFLAGS  = -DHAVE_CXX_STDHEADERS -D_GNU_SOURCE -D_REENTRANT -g -std=c++17
LDFLAGS  += -L/usr/local/db6/lib
LDLIBS    = -ldb_cxx -lsqlparser

SRC_DIR  := src
INC_DIR  := include
OBJ_DIR  := obj

# following is a list of all the compiled object files needed to build the sql5300 executable
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
HEADERS := $(wildcard $(INC_DIR)/*.h)
OBJS := $(subst $(SRC_DIR),$(OBJ_DIR),$(SRCS:.cpp=.o))

.PHONY: all
all: sql5300

# Rule for linking to create the executable
# Note that this is the default target since it is the first non-generic one in the Makefile: $ make
sql5300: $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

# General rules for compilation
# Just assume that every .cpp file depends on every header and the Makefile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS) Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Rule for removing all non-source files (so they can get rebuilt from scratch)
# Note that since it is not the first target, you have to invoke it explicitly: $ make clean
.PHONY: clean
clean:
	$(RM) sql5300 $(OBJ_DIR)/*.o

