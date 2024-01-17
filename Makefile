# # Compiler
# CXX = g++

# # Compiler flags
# CXXFLAGS = -std=c++11 -Wall

# # Source file
# SOURCE = src/main.cpp

# # Executable name
# TARGET = main

# all: $(TARGET)

# $(TARGET): $(SOURCE)
# 	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

# clean:
# 	rm -f $(TARGET)

# ############################# with classes

# # Compiler and flags
# CXX = g++
# CXXFLAGS = -std=c++11 -Wall

# # Directories
# SRCDIR = src
# OBJDIR = obj
# BINDIR = bin

# # Source files
# SRCS = $(wildcard $(SRCDIR)/*.cpp)
# OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# # Executable
# TARGET = $(BINDIR)/my_project

# # Default target
# all: $(TARGET)

# # Rule to build the executable
# $(TARGET): $(OBJS)
# 	@mkdir -p $(BINDIR)
# 	$(CXX) $(CXXFLAGS) $^ -o $@

# # Rule to compile source files
# $(OBJDIR)/%.o: $(SRCDIR)/%.cpp
# 	@mkdir -p $(OBJDIR)
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# # Clean rule
# clean:
# 	rm -rf $(OBJDIR) $(BINDIR)

# # Phony target to prevent conflicts with file names
# .PHONY: all clean


# ############################ for milestone 1 version 0
# # Compiler and flags
# CXX = g++
# CXXFLAGS = -I/usr/local/db6/include -DHAVE_CXX_STDHEADERS -D_GNU_SOURCE -D_REENTRANT -O3 -std=c++11
# LDFLAGS = -L/usr/local/db6/lib
# LIBS = -ldb_cxx -lsqlparser

# # Source file
# SOURCE = main.cpp SQLprinting.cpp
# SRCDIR = src

# # Executable name
# TARGET = main

# all: $(TARGET)

# $(TARGET): $(SOURCE)
# 	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

# clean:
# 	rm -f $(TARGET)



############################# version 1 works 

# Compiler and flags
CXX = g++
CXXFLAGS = -I/usr/local/db6/include -DHAVE_CXX_STDHEADERS -D_GNU_SOURCE -D_REENTRANT -O3 -std=c++11
LDFLAGS = -L/usr/local/db6/lib
LIBS = -ldb_cxx -lsqlparser

# Source files
SRCS = src/main.cpp src/SQLprinting.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Executable name
EXEC = my_program

# Rule to build executable
$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

# Rule to build object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean rule
clean:
	rm -f $(EXEC) $(OBJS)
