#ifndef SQLPRINTING_H
#define SQLPRINTING_H
#include <stdio.h>
#include <string>
#include "sql/statements.h"

using namespace hsql;

class SQLprinting
{
public:

  void printingStatement(const SQLStatement* stmt);

private:

  void printCreateStatementInfo(const CreateStatement* stmt);

  void printSelectStatementInfo(const SelectStatement* stmt);

  std::string printingExpression(Expr* expr);

  std::string printingTableRefInfo(TableRef* table);

  std::string printingOperatorExpression(Expr* expr);

  std::string columnDefinitionToString(const ColumnDefinition *col); 

};

#endif