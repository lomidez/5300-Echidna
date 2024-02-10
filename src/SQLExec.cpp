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

// make query result be printable
ostream &operator<<(ostream &out, const QueryResult &qres) {
    if (qres.column_names != nullptr) {
        for (auto const &column_name: *qres.column_names)
            out << column_name << " ";
        out << endl << "+";
        for (unsigned int i = 0; i < qres.column_names->size(); i++)
            out << "----------+";
        out << endl;
        for (auto const &row: *qres.rows) {
            for (auto const &column_name: *qres.column_names) {
                Value value = row->at(column_name);
                switch (value.data_type) {
                    case ColumnAttribute::INT:
                        out << value.n;
                        break;
                    case ColumnAttribute::TEXT:
                        out << "\"" << value.s << "\"";
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

QueryResult::~QueryResult() {
    delete column_names;
    delete column_attributes;
    if (rows != nullptr) {
        for (auto row : *rows) {
            delete row; // Delete each dynamically allocated ValueDict object
        }
        delete rows; // Then delete the vector itself
    }
}

QueryResult *SQLExec::execute(const SQLStatement *statement) {
    if (tables == nullptr) {
        initialize_schema_tables();
        tables = new Tables();
    }

    try {
        switch (statement->type()) {
            case kStmtCreate:
                return create((const CreateStatement *) statement);
            case kStmtDrop:
                return drop((const DropStatement *) statement);
            case kStmtShow:
                return show((const ShowStatement *) statement);
            default:
                return new QueryResult("not implemented");
        }
    } catch (DbRelationError &e) {
        throw SQLExecError(string("DbRelationError: ") + e.what());
    }
}

void
SQLExec::column_definition(const ColumnDefinition *col, Identifier &column_name, ColumnAttribute &column_attribute) {
    column_name = col->name;
    if(col->type == ColumnDefinition::INT) {
        column_attribute.set_data_type(ColumnAttribute::INT);
    } else if (col->type == ColumnDefinition::TEXT) {
        column_attribute.set_data_type(ColumnAttribute::TEXT);
    } else {
        throw SQLExecError("Column Attribute Type Not Supported");
    }
}

QueryResult *SQLExec::create(const CreateStatement *statement) {
    // get column attributes after conversion from hsql definition to col attribute
    ColumnNames column_names;
    ColumnAttributes column_attr;
    Identifier curr_column_name;
    ColumnAttribute curr_column_attribute;
    for (ColumnDefinition *col : *statement->columns) {
        column_definition(col, curr_column_name, curr_column_attribute);
        column_names.push_back(curr_column_name);
        column_attr.push_back(curr_column_attribute);
    }

    // Add to _tables and _columns
    Identifier table_name = statement->tableName;
    ValueDict row;
    row["table_name"] = table_name;
    Handle table_handle;
    try {
        table_handle = tables->insert(&row); 
        DbRelation *table = &tables->get_table(table_name);
        DbRelation &col_table = tables->get_table(Columns::TABLE_NAME);

        Handles column_handles;
        try {
            for (uint i = 0; i < column_names.size(); i++) {
                row["column_name"] = column_names.at(i);
                // row["data_type"] = column_attr.at(i) == ColumnAttribute::INT? Value("INT") : Value("TEXT");
                row["data_type"] = Value(column_attr[i].get_data_type() == ColumnAttribute::INT ? "INT" : "TEXT");
                Handle curr_col_handle = col_table.insert(&row);
                column_handles.push_back(curr_col_handle);
            }
            if(statement->ifNotExists) {
                table->create_if_not_exists();
            } else {
                table->create();
            }
        } catch(...) {
            //rollback columns
            try {
                for (auto const &handle: column_handles)
                    col_table.del(handle);
            } catch (...) {
                throw;
            }
        }
    } catch (...) {
        tables->del(table_handle);
    }
    return new QueryResult("created " + table_name);
}

// DROP ...
QueryResult *SQLExec::drop(const DropStatement *statement) {
    return new QueryResult("not implemented"); // FIXME
}

QueryResult *SQLExec::show(const ShowStatement *statement) {
    switch(statement->type) {
        case ShowStatement::kTables:
            return show_tables();
        case ShowStatement::kColumns:
            return show_columns(statement);
        default:
            throw SQLExecError(string("Operation Not Allowed"));
    }
}

QueryResult *SQLExec::show_tables() {
   return new QueryResult("not implemented"); // FIXME
}

QueryResult *SQLExec::show_columns(const ShowStatement *statement) {
    return new QueryResult("not implemented"); // FIXME
}


