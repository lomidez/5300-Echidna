/*
    File: SQLprinting.cpp
    Author: Lisa Lomidze
    Date: January 15, 2024
    Description: Implementation file for the SQLprinting class. 
    Defines member functions for printing information about 
    SQL statements. Also includes helper functions for 
    printing expressions, table references, and column definitions.
*/

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sql/statements.h"
#include "SQLprinting.h"
#include "SQLParser.h"

using namespace hsql;

void SQLprinting::printCreateStatementInfo(const CreateStatement* stmt)
{
  std::string outputStatement;
  outputStatement += "CREATE TABLE ";
  outputStatement += stmt->tableName;
  outputStatement += " (";
  for (ColumnDefinition* col : *stmt->columns) 
  {
      outputStatement += SQLprinting::columnDefinitionToString(col);
      outputStatement += ", ";
  }
  outputStatement.erase(outputStatement.size() - 2);
  outputStatement += ")";

  std::cout << outputStatement << std::endl;
}


void SQLprinting::printSelectStatementInfo(const SelectStatement* stmt)
{
  std::string outputStatement;
  outputStatement += "SELECT ";

  for (Expr* exprPtr : *stmt->selectList) 
  {
      outputStatement += SQLprinting::printingExpression(exprPtr);
      outputStatement.erase(outputStatement.size() - 1);
      outputStatement += ", ";
  }

  outputStatement.erase(outputStatement.size() - 2);

  if (stmt->fromTable)
  {
    outputStatement += " FROM ";
    outputStatement += SQLprinting::printingTableRefInfo(stmt->fromTable);
  }

  if (stmt->whereClause != NULL)
  {
    outputStatement += "WHERE ";
    outputStatement += SQLprinting::printingExpression(stmt->whereClause);
  }

  std::cout << outputStatement << std::endl;
}

std::string SQLprinting::printingExpression(Expr *expr) 
{
  std::string outputStr;
  switch (expr->type) 
  {
  case kExprStar:
    outputStr += "*";
    outputStr += " ";
    break;
  case kExprColumnRef:
    if (expr->table)
    {
      outputStr += expr->table;
      outputStr += ".";
      outputStr += expr->name;
      outputStr += " ";
    }
    else
    {
      outputStr += expr->name;
      outputStr += " ";
    }
    break;
  case kExprLiteralFloat:
    outputStr += std::to_string(expr->fval);
    outputStr += " ";
    break;
  case kExprLiteralInt:
    outputStr += std::to_string(expr->ival);
    outputStr += " ";
    break;
  case kExprLiteralString:
    outputStr += expr->name;
    outputStr += " ";
    break;
  case kExprOperator:
    outputStr += printingOperatorExpression(expr);
    break;
  default:
    fprintf(stderr, "Unrecognized expression type %d\n", expr->type);
  }
  if (expr->alias != NULL) 
  {
    outputStr += "AS ";
    outputStr += expr->alias;
    outputStr += " ";
  }
  return outputStr;
}

std::string SQLprinting::printingOperatorExpression(Expr *expr) {
  std::string outputStr;
  outputStr += SQLprinting::printingExpression(expr->expr);
  if (expr == NULL) {
    outputStr += "null";
    return outputStr;
  }
  switch (expr->opType) {
  case Expr::SIMPLE_OP:
    outputStr += expr->opChar;
    outputStr += " ";
    break;
  case Expr::AND:
    outputStr += " AND ";
    break;
  case Expr::OR:
    outputStr += " OR ";
    break;
  case Expr::NOT:
    outputStr += " NOT ";
    break;
  default:
    outputStr += expr->opType;
    break;
  }
  if (expr->expr2 != NULL){
    outputStr += SQLprinting::printingExpression(expr->expr2);
  }
  return outputStr;
}


std::string SQLprinting::printingTableRefInfo(TableRef *table) 
{
  std::string outputStr = "";
  switch (table->type) 
  {
  case kTableName:
    outputStr += table->name;
    outputStr += " ";
    break;
  case kTableJoin:
    outputStr += SQLprinting::printingTableRefInfo(table->join->left);
    if (table->join->type == kJoinLeft)
    {
      outputStr += "LEFT ";
    }
    else if (table->join->type == kJoinRight)
    {
      outputStr += "RIGHT ";
    }
    outputStr += "JOIN ";
    outputStr += SQLprinting::printingTableRefInfo(table->join->right);
    outputStr += "ON ";
    outputStr += SQLprinting::printingExpression(table->join->condition);
    break;
  case kTableCrossProduct:
    for (TableRef* tbl : *table->list)
    {
      outputStr += printingTableRefInfo(tbl);
      outputStr.erase(outputStr.size() - 1);
      outputStr += ", ";
    }
    outputStr.erase(outputStr.size() - 2);
    break;
  }
  if (table->alias != NULL) {
    outputStr += "AS ";
    outputStr += table->alias;
    outputStr += " ";
  }
  return outputStr;
}

std::string SQLprinting::columnDefinitionToString(const ColumnDefinition *col) 
{
  std::string ret(col->name);
  switch(col->type) {
  case ColumnDefinition::DOUBLE:
      ret += " DOUBLE";
      break;
  case ColumnDefinition::INT:
      ret += " INT";
      break;
  case ColumnDefinition::TEXT:
      ret += " TEXT";
      break;
  default:
      ret += " ...";
      break;
  }
  return ret;
}

void SQLprinting::printingStatement(const SQLStatement* stmt) 
{
  switch (stmt->type()) 
  {
    case kStmtSelect:
      printSelectStatementInfo((const SelectStatement*) stmt);
      break;
    case kStmtCreate:
      printCreateStatementInfo((const CreateStatement*) stmt);
      break;
    default:
      break;
  }

}

