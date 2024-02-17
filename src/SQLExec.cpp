/**
 * @file SQLExec.cpp - implementation of SQLExec class
 * @author Kevin Lundeen
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

QueryResult *SQLExec::create_index(const CreateStatement *statement)
{
    Identifier table_name = statement->tableName;
    Identifier index_name = statement->indexName;
    Identifier index_type = statement->indexType;
    bool is_unique = index_type == "BTREE" ? true : false;

    ColumnNames *table_column_names = new ColumnNames();
    ColumnAttributes *table_column_attributes = new ColumnAttributes();
    tables->get_columns(table_name, *table_column_names, *table_column_attributes);
    delete table_column_attributes; // we don't need the column attributes

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

// DROP ...
QueryResult *SQLExec::drop(const DropStatement *statement)
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

    // Delete entries from _columns
    ValueDict where = {{"table_name", Value(table_name)}};
    DbRelation &columns = tables->get_table(Columns::TABLE_NAME);
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

QueryResult *SQLExec::drop_index(const DropStatement *statement)
{
    return new QueryResult("drop index not implemented"); // FIXME
}
