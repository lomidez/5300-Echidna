/*
    File: main.cpp
    Authors: Kevin Lundeen, Lisa Lomidze
    Date: January 27, 2024
    Description: Main program file for the SQL interpreter. 
    Initializes Berkeley DB environment, takes user input for SQL statements, 
    parses and prints the statements using the SQLprinting class. Allows the user 
    to interactively input SQL statements until the user enters "quit". Allows to test 
    functionality of heap storage if user enters "test".
*/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <cassert>
#include "db_cxx.h"
#include "SQLParser.h"
#include "SQLprinting.h"
#include "heap_storage.h"


using namespace std;
using namespace hsql;

/*
 * we allocate and initialize the _DB_ENV global
 */
DbEnv *_DB_ENV;

/**
 * Tests the functionality of heap storage.
 * @returns     true if all tests pass; otherwise, false.
 */
bool test_heap_storage() {
    std::cout << "I'm in test_heap_storage function" <<"\n";
    ColumnNames column_names;
    column_names.push_back("a");
    column_names.push_back("b");
    ColumnAttributes column_attributes;
    ColumnAttribute ca(ColumnAttribute::INT);
    column_attributes.push_back(ca);
    ca.set_data_type(ColumnAttribute::TEXT);
    column_attributes.push_back(ca);

    HeapTable table1("_test_create_drop_cpp", column_names, column_attributes);
    table1.create();
    std::cout << "create ok" << std::endl;
    table1.drop();  // drop makes the object unusable because of BerkeleyDB restriction -- maybe want to fix this some day
    std::cout << "drop ok" << std::endl;

    HeapTable table("_test_data_cpp", column_names, column_attributes);
    table.create_if_not_exists();
    std::cout << "create_if_not_exsts ok" << std::endl;

    ValueDict row;
    row["a"] = Value(12);
    row["b"] = Value("Hello!");

    std::cout << "try insert" << std::endl;
    table.insert(&row);
    std::cout << "insert ok" << std::endl;

    Handles* handles = table.select();
    std::cout << "select ok " << handles->size() << std::endl;

    ValueDict *result = table.project((*handles)[0]);
    std::cout << "project ok" << std::endl;


    Value value = (*result)["a"];
    if (value.n != 12)
        return false;
    value = (*result)["b"];
    if (value.s != "Hello!")
        return false;

    table.drop();

    return true;
}

/**
 * Main entry point of the sql5300 program
 * @args dbenvpath  the path to the BerkeleyDB database environment
 */
int main(int argc, char *argv[]) {

    // Open/create the db enviroment
    if (argc != 2) {
        cerr << "Usage: cpsc5300: dbenvpath" << endl; // /home/st/llomidze/cpsc5300/data
        return 1;
    }

    char *envHome = argv[1];
    cout << "(sql5300: running with database environment at " << envHome << ")" << endl;
    DbEnv env(0U);
    env.set_message_stream(&cout);
    env.set_error_stream(&cerr);

    try {
        env.open(envHome, DB_CREATE | DB_INIT_MPOOL, 0);
    } catch (DbException &exc) {
        cerr << "(sql5300: " << exc.what() << ")";
        exit(1);
    }

    _DB_ENV = &env;

    while (true) 
    {
        cout << "SQL> ";
        string query;
        getline(cin, query);

        if (query.length() == 0)
            continue;  // blank line -- just skip

        if (query == "quit")
            break;  // only way to get out

        if (query == "test") {
            cout << "test_heap_storage: " << (test_heap_storage() ? "ok" : "failed") << endl;
            continue;
        }

        // use the Hyrise sql parser to get us our AST
        SQLParserResult *result = SQLParser::parseSQLString(query);
        if (!result->isValid()) {
            cout << "invalid SQL: " << query << endl;
            delete result;
            continue;
        }


        // execute the statement
        for (uint i = 0; i < result->size(); ++i) {
            SQLprinting sqlprinting;
            sqlprinting.printingStatement(result->getStatement(i));
        }
        delete result;

    }

    return EXIT_SUCCESS;
}
