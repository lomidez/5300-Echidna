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
HEAP_STORAGE_H = include/heap_storage.h include/SlottedPage.h include/HeapFile.h include/HeapTable.h include/storage_engine.h
SCHEMA_TABLES_H = include/schema_tables.h $(HEAP_STORAGE_H)
SQLEXEC_H = include/SQLExec.h $(SCHEMA_TABLES_H)
src/ParseTreeToString.o : include/ParseTreeToString.h
src/SQLExec.o : $(SQLEXEC_H)
src/SlottedPage.o : include/SlottedPage.h
src/HeapFile.o : include/HeapFile.h include/SlottedPage.h
src/HeapTable.o : $(HEAP_STORAGE_H)
src/schema_tables.o : $(SCHEMA_TABLES_H) include/ParseTreeToString.h
src/sql5300.o : $(SQLEXEC_H) include/ParseTreeToString.h
src/storage_engine.o : include/storage_engine.h

# General rule for compilation with source files in src/ and include files in include/
%.o: %.cpp
	g++ -I$(INCLUDE_DIR) -I./include $(CCFLAGS) -o "$@" "$<"

# Rule for cleaning all non-source files
clean:
	rm -f $(EXEC) $(OBJS)
