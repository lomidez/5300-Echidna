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
