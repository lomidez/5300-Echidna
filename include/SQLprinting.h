/*
    File: SQLprinting.h
    Author: Lisa Lomidze
    Date: January 15, 2024
    Description: Header file for the SQLprinting class, 
    which provides functions to print information about SQL 
    statements. Includes necessary dependencies and declares 
    member functions for printing various types of SQL statements.
*/

#include <stdio.h>
#include <string>
#include "sql/statements.h"

using namespace hsql;

class SQLprinting
{
public:

  /**
    * Print information about the given SQL statement.
    * @param stmt Pointer to the SQL statement to print.
    */
  void printingStatement(const SQLStatement* stmt);

private:

  /**
    * Print information about the CREATE statement.
    * @param stmt Pointer to the CREATE statement.
    */
  void printCreateStatementInfo(const CreateStatement* stmt);

  /**
    * Print information about the SELECT statement.
    * @param stmt Pointer to the SELECT statement.
    */
  void printSelectStatementInfo(const SelectStatement* stmt);

  /**
    * Convert an expression to its string representation.
    * @param expr Pointer to the expression.
    * @return String representation of the expression.
    */
  std::string printingExpression(Expr* expr);

  /**
    * Convert an operator expression to its string representation.
    * @param expr Pointer to the operator expression.
    * @return String representation of the operator expression.
    */
  std::string printingOperatorExpression(Expr* expr);

  /**
    * Convert a TableRef to its string representation.
    * @param table Pointer to the TableRef.
    * @return String representation of the TableRef.
    */
  std::string printingTableRefInfo(TableRef* table);

  /**
    * Convert the hyrise ColumnDefinition AST back into the equivalent SQL
    * @param col  column definition to unparse
    * @return     SQL equivalent to *col
   */
  std::string columnDefinitionToString(const ColumnDefinition *col); 

};