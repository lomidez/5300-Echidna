# Adapted from Kevin Lundeen's Makefile, Seattle University

CCFLAGS     = -std=c++11 -Wall -Wno-c++11-compat -DHAVE_CXX_STDHEADERS -D_GNU_SOURCE -D_REENTRANT -O3 -c -ggdb
COURSE      = /usr/local/db6
INCLUDE_DIR = $(COURSE)/include
LIB_DIR     = $(COURSE)/lib

# Paths for source files located in src/ directory
OBJS        = src/sql5300.o src/SlottedPage.o src/HeapFile.o src/HeapTable.o src/ParseTreeToString.o src/SQLExec.o src/schema_tables.o src/storage_engine.o

# Executable name 
EXEC        = sql5300

# Rule for linking to create the executable
$(EXEC): $(OBJS)
	g++ -L$(LIB_DIR) -o $@ $(OBJS) -ldb_cxx -lsqlparser

# Header file dependencies 
HEAP_STORAGE_H = src/heap_storage.h src/SlottedPage.h src/HeapFile.h src/HeapTable.h src/storage_engine.h
SCHEMA_TABLES_H = src/schema_tables.h $(HEAP_STORAGE_H)
SQLEXEC_H = src/SQLExec.h $(SCHEMA_TABLES_H)
src/ParseTreeToString.o : src/ParseTreeToString.h
src/SQLExec.o : $(SQLEXEC_H)
src/SlottedPage.o : src/SlottedPage.h
src/HeapFile.o : src/HeapFile.h src/SlottedPage.h
src/HeapTable.o : $(HEAP_STORAGE_H)
src/schema_tables.o : $(SCHEMA_TABLES_H) src/ParseTreeToString.h
src/sql5300.o : $(SQLEXEC_H) src/ParseTreeToString.h
src/storage_engine.o : src/storage_engine.h

# General rule for compilation with source files in src/
%.o: %.cpp
	g++ -I$(INCLUDE_DIR) $(CCFLAGS) -o "$@" "$<"

# Rule for cleaning all non-source files
clean:
	rm -f $(EXEC) $(OBJS)
