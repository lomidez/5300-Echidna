#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SQLprinting.h"
#include "db_cxx.h"
#include "SQLParser.h"

using namespace hsql;

const unsigned int BLOCK_SZ = 4096;

int main(void) {

    std::cout << "Enter the dir: ";
    std::string userHomeDir;
    std::cin >> userHomeDir;

    const char *HOME = userHomeDir.c_str();

    const char *home = std::getenv("HOME");
    std::string envdir = std::string(home) + "/" + HOME;

    // Initializing Berkeley DB environment
    DbEnv env(0U);
    env.set_message_stream(&std::cout);
    env.set_error_stream(&std::cerr);
    env.open(envdir.c_str(), DB_CREATE | DB_INIT_MPOOL, 0);


    std::string inputStatement;
    while (true) 
    {
        std::cout << "SQL> ";
        std::getline(std::cin, inputStatement);

        if (inputStatement == "quit") 
        {
            break;
        }

        // parse a given query
        hsql::SQLParserResult* result = hsql::SQLParser::parseSQLString(inputStatement);

        // check whether the parsing was successful
        if (result->isValid()) 
        {

            for (uint i = 0; i < result->size(); ++i) 
            {
                SQLprinting sqlprinting;
                sqlprinting.printingStatement(result->getStatement(i));
            }
        }
        else
        {
            std::cout << "Invalid SQL: " << inputStatement << "\n";
        }

    }

    return EXIT_SUCCESS;
}
