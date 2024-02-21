/**
 * @file SQLExec.cpp - implementation of SQLExec class
 * @author Kevin Lundeen, Noha Nomier, Dhruv Patel
 * @see "Seattle University, CPSC5300, Winter Quarter 2024"
 */
#include "SQLExec.h"

using namespace std;
using namespace hsql;

// define static data
Tables *SQLExec::tables = nullptr;
Indices *SQLExec::indices = nullptr;

// make query result be printable
ostream &operator<<(ostream &out, const QueryResult &qres)
{
    if (qres.column_names != nullptr)
    {
        for (auto const &column_name : *qres.column_names)
            out << column_name << " ";
        out << endl
            << "+";
        for (unsigned int i = 0; i < qres.column_names->size(); i++)
            out << "----------+";
        out << endl;
        for (auto const &row : *qres.rows)
        {
            for (auto const &column_name : *qres.column_names)
            {
                Value value = row->at(column_name);
                switch (value.data_type)
                {
                case ColumnAttribute::INT:
                    out << value.n;
                    break;
                case ColumnAttribute::TEXT:
                    out << "\"" << value.s << "\"";
                    break;
                case ColumnAttribute::BOOLEAN:
                    out << (value.n == 0 ? "false" : "true");
                    break;
                default:
                    out << "???";
                }
                out << " ";
            }
            out << endl;
        }
    }
    out << qres.message;
    return out;
}

QueryResult::~QueryResult()
{
    delete column_names;
    delete column_attributes;
    if (rows != nullptr)
    {
        for (auto row : *rows)
        {
            delete row; // Delete each dynamically allocated ValueDict object
        }
        delete rows; // Then delete the vector itself
    }
}

/**
 * Executes a given SQL statement and returns the result as a QueryResult object.
 * It also initializes the schema tables if they haven't been initialized yet.
 *
 * @param statement Pointer to a SQLStatement object representing the SQL statement to execute.
 * @return Pointer to a QueryResult object containing the outcome of the executed statement.
 * @throws SQLExecError if an error occurs during statement execution.
 */
QueryResult *SQLExec::execute(const SQLStatement *statement)
{
    if (tables == nullptr)
    {
        // initialize_schema_tables();
        tables = new Tables();
    }

    if (SQLExec::indices == nullptr)
    {
        indices = new Indices();
    }

    try
    {
        switch (statement->type())
        {
        case kStmtCreate:
            return create((const CreateStatement *)statement);
        case kStmtDrop:
            return drop((const DropStatement *)statement);
        case kStmtShow:
            return show((const ShowStatement *)statement);
        default:
            return new QueryResult("not implemented");
        }
    }
    catch (DbRelationError &e)
    {
        throw SQLExecError(string("DbRelationError: ") + e.what());
    }
}

/**
 * Extracts column definition details from an hsql::ColumnDefinition object.
 *
 * @param col Pointer to an hsql::ColumnDefinition object
 * @param column_name Reference to a string where the column's name will be stored.
 * @param column_attribute Reference to a ColumnAttribute object where the column's data type will be stored.
 * @throws SQLExecError if the column data type is not supported.
 */
void SQLExec::column_definition(const ColumnDefinition *col, Identifier &column_name, ColumnAttribute &column_attribute)
{
    column_name = col->name;
    if (col->type == ColumnDefinition::INT)
    {
        column_attribute.set_data_type(ColumnAttribute::INT);
    }
    else if (col->type == ColumnDefinition::TEXT)
    {
        column_attribute.set_data_type(ColumnAttribute::TEXT);
    }
    else
    {
        throw SQLExecError("Column Attribute Type Not Supported");
    }
}

/**
 * Handles the CREATE statement, it supports two types CREATE TABLE and CREATE INDEX
 *
 * @param statement Pointer to a CreateStatement object.
 * @return Pointer to a QueryResult object containing the outcome of the CREATE operation.
 */
QueryResult *SQLExec::create(const CreateStatement *statement)
{
    switch (statement->type)
    {
    case CreateStatement::CreateType::kTable:
        return create_table(statement);
    case CreateStatement::CreateType::kIndex:
        return create_index(statement);
    default:
        return new QueryResult("not implemented");
    }
}

/**
 * Handles the CREATE TABLE statement and physically creates a new table in the database based
 * on the specs in the statement. It updates the schema tables (_tables,_columns) accordingly.
 *
 * @param statement Pointer to a CreateStatement object specifying the table to create.
 * @return Pointer to a QueryResult object indicating the success of the operation
 * @throws DbRelationError if an error occurs during table creation.
 */
QueryResult *SQLExec::create_table(const CreateStatement *statement)
{
    // get column attributes after conversion from hsql definition to col attribute
    ColumnNames column_names;
    ColumnAttributes column_attr;
    Identifier curr_column_name;
    ColumnAttribute curr_column_attribute;
    for (ColumnDefinition *col : *statement->columns)
    {
        column_definition(col, curr_column_name, curr_column_attribute);
        column_names.push_back(curr_column_name);
        column_attr.push_back(curr_column_attribute);
    }

    // Add to _tables and _columns
    Identifier table_name = statement->tableName;
    ValueDict row;
    row["table_name"] = table_name;
    Handle table_handle;
    table_handle = tables->insert(&row);
    Handles column_handles;
    DbRelation &table = tables->get_table(table_name);
    DbRelation &col_table = tables->get_table(Columns::TABLE_NAME);
    try
    {
        for (uint i = 0; i < column_names.size(); i++)
        {
            row["column_name"] = column_names[i];
            row["data_type"] = Value(column_attr[i].get_data_type() == ColumnAttribute::INT ? "INT" : "TEXT");
            Handle curr_col_handle = col_table.insert(&row);
            column_handles.push_back(curr_col_handle);
        }
        if (statement->ifNotExists)
        {
            table.create_if_not_exists();
        }
        else
        {
            table.create();
        }
        // delete &table;
        // delete &col_table;
    }
    catch (DbRelationError &e)
    {
        tables->del(table_handle);
        for (auto const &handle : column_handles)
            col_table.del(handle);
        throw;
    }
    return new QueryResult("created " + table_name);
}

/**
 * Handles the CREATE INDEX statement and physically creates a new table in the database based on the specs in the statement.
 * It updates schema tables (_tables,_columns,_indices)
 * @param statement Pointer to a CreateStatement object specifying the index to create.
 * @return Pointer to a QueryResult object indicating the success of the index creation.
 * @throws DbRelationError if an error occurs during index creation.
 */
QueryResult *SQLExec::create_index(const CreateStatement *statement)
{
    Identifier table_name = statement->tableName;
    Identifier index_name = statement->indexName;
    Identifier index_type = statement->indexType;
    bool is_unique = index_type == "BTREE" ? true : false;

    // use unique_ptr to automatically release the memory when they go out of scope
    unique_ptr<ColumnNames> table_column_names(new ColumnNames());
    unique_ptr<ColumnAttributes> table_column_attributes(new ColumnAttributes());
    tables->get_columns(table_name, *table_column_names, *table_column_attributes);
    
    if (table_column_names->size() == 0)
    {
        throw SQLExecError(string("Table ") + table_name + string(" doesn't exist"));
    }

    // Validate that the referenced columns exist in the underlying table
    for (const auto &col_name : *statement->indexColumns)
    {
        auto it = std::find(table_column_names->begin(), table_column_names->end(), col_name);
        if (it == table_column_names->end())
        {
            throw SQLExecError(string("Column ") + col_name + string(" doesn't exist in ") + table_name);
        }
    }

    int seq_in_index = 1; // start counter for the index columns
    Handles index_handles;
    ValueDict row;
    // these are constant for all the referenced columns
    row["table_name"] = Value(table_name);
    row["index_name"] = Value(index_name);
    row["index_type"] = Value(index_type);
    row["is_unique"] = Value(is_unique);

    // insertion

    try
    {
        for (const auto &column_name : *statement->indexColumns)
        {
            row["seq_in_index"] = Value(seq_in_index);
            row["column_name"] = Value(column_name);
            index_handles.push_back(SQLExec::indices->insert(&row));
            seq_in_index++;
        }

        DbIndex *index = &indices->get_index(table_name, index_name);
        index->create();
    }
    catch (DbRelationError &exc)
    {
        for (Handle handle : index_handles)
        {
            indices->del(handle);
        }
        throw exc;
    }

    return new QueryResult("created index " + index_name);
}

/**
 * Handles the display of tables, columns, or indexes based on a SHOW statement.
 *
 * @param statement Pointer to a ShowStatement object
 * @return Pointer to a QueryResult object containing the requested information.
 * @throws SQLExecError if the operation type is not allowed or not implemented.
 */
QueryResult *SQLExec::show(const ShowStatement *statement)
{
    switch (statement->type)
    {
    case ShowStatement::kTables:
        return show_tables();
    case ShowStatement::kColumns:
        return show_columns(statement);
    case ShowStatement::kIndex:
        return show_index(statement);
    default:
        throw SQLExecError(string("Operation Not Allowed"));
    }
}

/**
 * Handles the SHOW TABLES statement which displays the _tables table that contains
 * all the tables information in the database
 *
 * @return Pointer to a QueryResult object containing the requested information.
 */
QueryResult *SQLExec::show_tables()
{
    ColumnNames *column_names = new ColumnNames;
    ValueDicts *rows = new ValueDicts();

    Handles *handles = tables->select();
    for (const Handle &handle : *handles)
    {
        ValueDict *row = tables->project(handle, column_names);
        Identifier table_name = row->at("table_name").s;
        if (table_name != Tables::TABLE_NAME && table_name != Columns::TABLE_NAME && table_name != Indices::TABLE_NAME)
        {
            rows->push_back(row);
        }
        else
        {
            delete row;
        }
    }
    ColumnAttributes *column_attributes = new ColumnAttributes();
    tables->get_columns(Tables::TABLE_NAME, *column_names, *column_attributes);

    delete handles;
    return new QueryResult(column_names, column_attributes, rows, "successfully returned " + to_string(rows->size()) + " rows");
}

/**
 *  Handles the SHOW COLUMNS FROM <TABLE> statement which displays the column data
 *  from the _columns table on the give table
 *
 * @param statement Pointer to a ShowStatement object specifying the table
 * @return Pointer to a QueryResult object containing the column information.
 */
QueryResult *SQLExec::show_columns(const ShowStatement *statement)
{
    ColumnNames *column_names = new ColumnNames({"table_name", "column_name", "data_type"});
    ColumnAttributes *column_attributes = new ColumnAttributes({ColumnAttribute(ColumnAttribute::DataType::TEXT)});

    // Check if specific table is requested
    if (statement->tableName != nullptr)
    {
        ValueDict where = {{"table_name", Value(statement->tableName)}};
        DbRelation &columns = tables->get_table(Columns::TABLE_NAME);
        Handles *rows = columns.select(&where);
        ValueDicts *data = new ValueDicts();
        for (Handle &row : *rows)
        {
            data->push_back(columns.project(row, column_names));
        }
        delete rows;
        return new QueryResult(column_names, column_attributes, data, "successfully returned " + to_string(data->size()) + " rows");
    }
    else
    {
        // If no table specified, retrieve all columns
        Handles *rows = tables->select();
        ValueDicts *data = new ValueDicts();
        for (Handle &row : *rows)
        {
            data->push_back(tables->project(row, column_names));
        }
        delete rows;
        return new QueryResult(column_names, column_attributes, data, "successfully returned " + to_string(data->size()) + " rows");
    }
}

/**
 *  Handles the SHOW INDEX FROM <TABLE> statement which displays the index data
 *  from the _indices table on the give table
 *
 * @param statement Pointer to a ShowStatement object specifying the table
 * @return Pointer to a QueryResult object containing the indices information.
 */
QueryResult *SQLExec::show_index(const ShowStatement *statement)
{
    // SHOW INDEX FROM <table_name>
    Identifier table_name = statement->tableName;

    ColumnNames *column_names = new ColumnNames();
    ColumnAttributes *column_attributes = new ColumnAttributes();
    tables->get_columns(Indices::TABLE_NAME, *column_names, *column_attributes); // get the column names and attr from indices

    ValueDicts *rows = new ValueDicts();
    ValueDict where;
    where["table_name"] = Value(table_name);

    Handles *index_handles = indices->select(&where); // select * from indices where table_name = <table_name>

    for (Handle handle : *index_handles)
    {
        rows->push_back(indices->project(handle));
    }
    string message = "successfully returned " + to_string(index_handles->size()) + " rows";
    delete index_handles;
    return new QueryResult(column_names, column_attributes, rows, message);
}

QueryResult *SQLExec::drop(const DropStatement *statement)
{
    switch (statement->type)
    {
    case DropStatement::kTable:
        return drop_table(statement);
    case DropStatement::kIndex:
        return drop_index(statement);
    default:
        throw SQLExecError("DROP type not implemented");
    }
}

/**
 * Handles the dropping of tables based on a DROP TABLE statement and handles
 * the necessary updates in schema tables.
 *
 * @param statement Pointer to a DropStatement object specifying the table to drop.
 * @return Pointer to a QueryResult object indicating the success of the table drop.
 * @throws SQLExecError if an error occurs during table drop.
 */
QueryResult *SQLExec::drop_table(const DropStatement *statement)
{
    if (statement->type != DropStatement::kTable)
    {
        throw SQLExecError("unrecognized DROP type");
    }

    // Get table name and check if it's a schema table
    Identifier table_name = statement->name;
    if (table_name == Tables::TABLE_NAME || table_name == Columns::TABLE_NAME)
    {
        throw SQLExecError("Cannot drop a schema table!");
    }

    ValueDict where = {{"table_name", Value(table_name)}};
    DbRelation &columns = tables->get_table(Columns::TABLE_NAME);

    // before dropping the table, drop each index on the table
    Handles *selected = SQLExec::indices->select(&where);
    for (Handle &row : *selected)
        SQLExec::indices->del(row);
    delete selected;

    // Delete entries from _columns
    Handles *rows = columns.select(&where);
    try
    {
        for (Handle &row : *rows)
        {
            columns.del(row);
        }
        delete rows;

        // Drop the actual table
        DbRelation &table = tables->get_table(table_name);
        table.drop();

        // Remove entry from _tables
        rows = tables->select(&where);
        tables->del(*rows->begin());
        delete rows;

        return new QueryResult(string("dropped ") + table_name);
    }
    catch (DbRelationError &e)
    {
        // Rollback changes if anything fails
        for (Handle &row : *rows)
        {
            try
            {
                ValueDict *rowData = columns.project(row, static_cast<const ColumnNames *>(nullptr));
                columns.insert(rowData); // Undo deletion from _columns
                delete rowData;          // Free the allocated memory
            }
            catch (DbRelationError &)
            {
            }
        }
        delete rows;
        throw; // Re-throw the original error
    }
}

/**
 * Handles the dropping of indexes based on a DROP INDEX statement.
 *
 * @param statement Pointer to a DropStatement object specifying the index to drop.
 * @return Pointer to a QueryResult object indicating the outcome of the drop index operation.
 */
QueryResult *SQLExec::drop_index(const DropStatement *statement)
{
    Identifier table_name = statement->name;
    Identifier index_name = statement->indexName;

    // Validate input
    if (table_name.empty() || index_name.empty()) {
        throw SQLExecError("Missing table or index name for DROP INDEX");
    }

    try {
        // Drop the actual index
        DbIndex *index = &indices->get_index(table_name, index_name);
        index->drop();

        // Remove entry from _indices
        ValueDict where = {{"table_name", Value(table_name)}, {"index_name", Value(index_name)}};
        Handles *index_handles = indices->select(&where);
        if (index_handles->empty()) {
            throw SQLExecError("Index not found");
        }

        // Remove entry from _indices
        for (Handle &handle : *index_handles) {
            indices->del(handle);
        }
        delete index_handles;

        return new QueryResult(string("dropped index ") + index_name);
    } catch (DbRelationError &e) {
        throw SQLExecError(string("DbRelationError: ") + e.what());
    }
}
