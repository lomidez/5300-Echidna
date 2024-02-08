/*
 	File: heap_storage.cpp
 	Authors: Lisa Lomidze
 	Date: January 27, 2024
 	Description: Implementation file for the heap storage engine, 
 	including SlottedPage, HeapFile, and HeapTable classes.
 */

#include "heap_storage.h"
#include <cstring>

typedef u_int16_t u16;

//////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// SlottedPage ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


SlottedPage::SlottedPage(Dbt &block, BlockID block_id, bool is_new) : DbBlock(block, block_id, is_new) 
{
    if (is_new) 
    {
        this->num_records = 0;
        this->end_free = DbBlock::BLOCK_SZ - 1;
        put_header();
    } 
    else 
    {
        get_header(this->num_records, this->end_free);
    }
}

// Add a new record to the block. Return its id.
RecordID SlottedPage::add(const Dbt *data)
{
    if (!has_room(data->get_size()))
        throw DbBlockNoRoomError("not enough room for new record");

    u16 id = ++this->num_records;
    u16 size = (u16) data->get_size();
    this->end_free -= size;
    u16 loc = this->end_free + 1;
    put_header();
    put_header(id, size, loc);
    memcpy(this->address(loc), data->get_data(), size);
    return id;
}


Dbt *SlottedPage::get(RecordID record_id)
{
	u16 size = 0;
	u16 loc = 0;
	get_header(size, loc, record_id);
	if (loc == 0)
	{
		return nullptr;
	}
	else
	{
		Dbt *data = new Dbt(address(loc), size);
        return data;	
	}
}

void SlottedPage::put(RecordID record_id, const Dbt &data)
{
	u16 size = 0;
	u16 loc = 0;
	get_header(size, loc, record_id);
	u16 new_size = data.get_size();
	if (new_size > size)
	{
		u16 extra = new_size - size;
		if(this->has_room(extra) == false)
		{
			throw DbBlockNoRoomError("Not enough room in block"); 
		}	
		this->slide(loc, loc - extra);
        memcpy(this->address(loc - extra), data.get_data(), new_size);
	}
	else
	{
		memcpy(this->address(loc), data.get_data(), new_size);
		this->slide(loc + new_size, loc + size);
	}
	get_header(size, loc, record_id);
	put_header(record_id, new_size, loc);
}

void SlottedPage::del(RecordID record_id)
{
	u16 size = 0;
	u16 loc = 0;
	get_header(size, loc, record_id);
	put_header(record_id, 0, 0);
	slide(loc, loc + size);
}

RecordIDs *SlottedPage::ids()
{
	u16 size = 0;
	u16 loc = 0;
	RecordIDs* recs = new RecordIDs();
	for (u16 i = 1; i <= this->num_records; i++)
	{
		get_header(size, loc, i);
		if (loc != 0)
		{
			recs->push_back(i);
		}
	}
	return recs;
}

void SlottedPage::get_header(u_int16_t &size, u_int16_t &loc, RecordID id)
{
	size = this->get_n(4*id);
    loc = this->get_n(4*id + 2);
}

// Store the size and offset for given id. For id of zero, store the block header.
void SlottedPage::put_header(RecordID id, u_int16_t size, u_int16_t loc)
{
    if (id == 0) { // called the put_header() version and using the default params
        size = this->num_records;
        loc = this->end_free;
    }
    put_n(4*id, size);
    put_n(4*id + 2, loc);
}


bool SlottedPage::has_room(u_int16_t size)
{
	u16 available = this->end_free - (this->num_records + 1) * 4;
	return (size <= available);
}


void SlottedPage::slide(u_int16_t start, u_int16_t end)
{
	u16 shift = end - start;
	if (shift == 0)
	{
		return;
	}

	//slide data
    void *to = this->address(this->end_free + 1 + shift);
    void *from = this->address(this->end_free + 1);
    int bytes = start - (this->end_free + 1);
    memmove(to, from, bytes);

	//fixup headers
	RecordIDs *all_recs = this->ids();
	for (RecordID &id : *all_recs)
	{
		u16 size = 0;
		u16 loc = 0;
		get_header(size, loc, id);
		if (loc <= start)
		{
			loc += shift;
			put_header(id, size, loc);
		}
	}
	this->end_free += shift;
	put_header();
}

// Get 2-byte integer at given offset in block.
u_int16_t SlottedPage::get_n(u_int16_t offset)
{
    return *(u16*)this->address(offset);
}

// Put a 2-byte integer at given offset in block.
void SlottedPage::put_n(u_int16_t offset, u_int16_t n)
{
    *(u16*)this->address(offset) = n;
}

// Make a void* pointer for a given offset into the data block.
void *SlottedPage::address(u_int16_t offset)
{
    return (void*)((char*)this->block.get_data() + offset);
}


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// HeapFile ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


void HeapFile::create(void)
{
	db_open(DB_CREATE | DB_EXCL);
	SlottedPage* new_block = this->get_new();
    delete new_block;
}

void HeapFile::drop(void)
{
	close();
	std::remove((this->name + ".db").c_str());
	Db(_DB_ENV, 0).remove((this->name + ".db").c_str(), NULL, 0);
}

void HeapFile::open(void)
{
	db_open();
}

void HeapFile::close(void)
{
	this->db.close(0);
	this->closed = true;
}

SlottedPage *HeapFile::get_new(void)
{
    char block[DbBlock::BLOCK_SZ];
    memset(block, 0, sizeof(block));
    Dbt data(block, sizeof(block));

    int block_id = ++this->last;
    Dbt key(&block_id, sizeof(block_id));

    // write out an empty block and read it back in so Berkeley DB is managing the memory
    SlottedPage* page = new SlottedPage(data, this->last, true);
    this->db.put(nullptr, &key, &data, 0); // write it out with initialization applied
    this->db.get(nullptr, &key, &data, 0);
    return page;
}

SlottedPage *HeapFile::get(BlockID block_id)
{
	Dbt key(&block_id, sizeof(block_id)), data;
  	this->db.get(nullptr, &key, &data, 0);
	SlottedPage* page = new SlottedPage(data, block_id, false);
	return page;
}

void HeapFile::put(DbBlock *block)
{
  BlockID block_id = block->get_block_id();
  Dbt key(&block_id, sizeof(block_id));
  this->db.put(nullptr, &key, block->get_block(), 0);
}

BlockIDs *HeapFile::block_ids()
{
	BlockIDs* bl_ids = new BlockIDs();
	for (u16 i = 1; i <= this->last; i++) 
	{
		bl_ids->push_back(i);
	}
	return bl_ids;
}

void HeapFile::db_open(uint flags)
{
	if (this->closed == false)
	{
		return;
	}
	this->db.set_re_len(DbBlock::BLOCK_SZ);
    db.open(NULL, (this->name + ".db").c_str(), NULL, DB_RECNO, flags, 0644);
	if (flags == 0) {
			DB_BTREE_STAT *stat;
			this->db.stat(nullptr, &stat, DB_FAST_STAT);
			this->last = stat->bt_ndata;
			free(stat);
		} else {
			this->last = 0;
		}
		this->closed = false;
}


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// HeapTable ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


HeapTable::HeapTable(Identifier table_name, ColumnNames column_names, ColumnAttributes column_attributes) :
DbRelation(table_name, column_names, column_attributes), file(table_name)
{

}

void HeapTable::create()
{
	this->file.create();
}

void HeapTable::create_if_not_exists()
{
	try
	{
		this->file.open();
	}
	catch (DbException &e) 
	{
        this->file.create();
    }
}

void HeapTable::drop()
{
	this->file.drop();
}

void HeapTable::open()
{
	this->file.open();
}

void HeapTable::close()
{
	this->file.close();
}

Handle HeapTable::insert(const ValueDict *row)
{
	this->open();
    ValueDict *full_row = validate(row);
    Handle handle = append(full_row);
    delete full_row;
    return handle;}

void HeapTable::update(const Handle handle, const ValueDict *new_values) //not milestone2
{
    throw DbRelationError("Not implemented");
}

void HeapTable::del(const Handle handle) //not milestone2
{	
    throw DbRelationError("Not implemented");
}

Handles *HeapTable::select() {
    ValueDict empty;
    return select(&empty);
}

Handles *HeapTable::select(const ValueDict *where) //not milestone2
{
    Handles* handles = new Handles();
    BlockIDs* block_ids = file.block_ids();
    for (auto const& block_id: *block_ids) {
        SlottedPage* block = file.get(block_id);
        RecordIDs* record_ids = block->ids();
        for (auto const& record_id: *record_ids)
            handles->push_back(Handle(block_id, record_id));
        delete record_ids;
        delete block;
    }
    delete block_ids;
    return handles;
}

//map<Identifier, Value> ValueDict
//Return a sequence of values for handle given by column_names.
ValueDict *HeapTable::project(Handle handle)
{
    return project(handle, &this->column_names);
}

ValueDict *HeapTable::project(Handle handle, const ColumnNames *column_names) //not milestone2
{
    BlockID block_id = handle.first;
    RecordID record_id = handle.second;
    SlottedPage *block = file.get(block_id);
    Dbt *data = block->get(record_id);
    ValueDict *row = unmarshal(data);
    delete data;
    delete block;
    if (column_names->empty())
        return row;
    ValueDict *result = new ValueDict();
    for (auto const &column_name: *column_names)
        (*result)[column_name] = (*row)[column_name];
    delete row;
    return result;
}

ValueDict *HeapTable::validate(const ValueDict *row)
{
  	ValueDict *full_row = new ValueDict();

  	for (auto const& column_name: this->column_names)
  	{
  		ValueDict::const_iterator column = row->find(column_name);
  		if (column == row->end())
  		{
  			throw DbRelationError("don't know how to handle NULLs, defaults, etc. yet");
  		}	
  		else
  		{
  			(*full_row)[column_name] = column->second;	
  		}
  	}
  	return full_row;
}

Handle HeapTable::append(const ValueDict *row)
{
	Dbt *data = marshal(row);
	BlockID block_id = this->file.get_last_block_id();
  	SlottedPage *block = this->file.get(block_id);
  	RecordID record_id;
  	try 
  	{
    	RecordID record_id = block->add(data);
  	} 
  	catch (DbException &e) 
  	{
    	block = this->file.get_new();
    	RecordID record_id = block->add(data);
 	}
  	this->file.put(block);
  	Handle new_handle(block->get_block_id(), record_id);
  	return new_handle;
}

// return the bits to go into the file
// caller responsible for freeing the returned Dbt and its enclosed ret->get_data().
Dbt *HeapTable::marshal(const ValueDict *row)
{
    char *bytes = new char[DbBlock::BLOCK_SZ]; // more than we need (we insist that one row fits into DbBlock::BLOCK_SZ)
    uint offset = 0;
    uint col_num = 0;
    for (auto const& column_name: this->column_names) {
        ColumnAttribute ca = this->column_attributes[col_num++];
        ValueDict::const_iterator column = row->find(column_name);
        Value value = column->second;
        if (ca.get_data_type() == ColumnAttribute::DataType::INT) {
            *(int32_t*) (bytes + offset) = value.n;
            offset += sizeof(int32_t);
        } else if (ca.get_data_type() == ColumnAttribute::DataType::TEXT) {
            uint size = value.s.length();
            *(u16*) (bytes + offset) = size;
            offset += sizeof(u16);
            memcpy(bytes+offset, value.s.c_str(), size); // assume ascii for now
            offset += size;
        } else {
            throw DbRelationError("Only know how to marshal INT and TEXT");
        }
    }
    char *right_size_bytes = new char[offset];
    memcpy(right_size_bytes, bytes, offset);
    delete[] bytes;
    Dbt *data = new Dbt(right_size_bytes, offset);
    return data;	
}

ValueDict *HeapTable::unmarshal(Dbt *data)
{
	char *bytes = (char *)data->get_data();
	ValueDict *full_row = new ValueDict();
	uint offset = 0;
	uint col_num = 0;
    for (auto const& column_name: this->column_names) 
    {
    	ColumnAttribute ca = this->column_attributes[col_num++];
    	if (ca.get_data_type() == ColumnAttribute::DataType::INT) 
    	{
  			(*full_row)[column_name] = Value(*(int32_t *)(bytes + offset));
            offset += sizeof(int32_t);
    	}
    	else if (ca.get_data_type() == ColumnAttribute::DataType::TEXT) 
    	{
    		uint size = *(u16 *)(bytes + offset); 
      		offset += sizeof(u16);
      		std::string value = std::string(bytes + offset, size);
      		offset += size;
      		(*full_row)[column_name] = Value(value);
    	}
    	else 
    	{
            throw DbRelationError("Only know how to unmarshal INT and TEXT");
        }
    }
    return full_row;
}








