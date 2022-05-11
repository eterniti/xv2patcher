#include "UtfLowlevel.h"
#include "debug.h"

void *UtfValue::GetValue()
{
	switch (type)
	{
		case TYPE_1BYTE: case TYPE_1BYTE2:
			return (void *)u1._u8;
		break;
		
		case TYPE_2BYTE: case TYPE_2BYTE2:
			return (void *)u1._u16;
		break;
		
		case TYPE_4BYTE: case TYPE_4BYTE2:
			return (void *)u1._u32;
		break;
		
		case TYPE_8BYTE: case TYPE_8BYTE2:
			return (void *)u1._u64;
		break;
		
		case TYPE_FLOAT:
			return (void *)u1._float;
		break;
		
		case TYPE_STRING:
			return (void *)u1.str;
		break;
		
		case TYPE_DATA:
			return (void *)u1.data;
		break;
	}
	
	return nullptr;
}

bool UtfFile::Load(const uint8_t *buf, size_t size)
{
	UTFHeader *phdr = (UTFHeader *)buf;
	
	uint32_t table_size = val32(phdr->table_size);	
	if (phdr->signature != UTF_SIGNATURE || table_size + sizeof(UTFHeader) < size)
		return false;
	
	table_hdr = (UTFTableHeader *)GetOffsetPtr(phdr, 8, true);	
	char *strings = (char *)GetOffsetPtr(table_hdr, table_hdr->strings_offset);
	uint8_t *data = GetOffsetPtr(table_hdr, table_hdr->data_offset);
	
	uint8_t *col_ptr = GetOffsetPtr(table_hdr, sizeof(UTFTableHeader), true);
		
	for (uint16_t i = 0; i < val16(table_hdr->num_columns); i++)
	{
		UtfColumn col;
		
		col.flags = *(col_ptr++);
		
		if (col.flags == 0)
		{
			col.flags = col_ptr[3];
			col_ptr += 4;
		}
		
		col.name = (char *)GetOffsetPtr(strings, *(uint32_t *)col_ptr);
		//DPRINTF("Col.name = %s\n", col.name);
		col_ptr += 4;
		columns.push_back(col);	

		// TODO: constants
	}
	
	UtfRow current_row;
	UtfValue current_value;
	uint32_t storage_flag;
	
	for (uint32_t j = 0; j < val32(table_hdr->num_rows); j++)
	{
		uint8_t *row_ptr = GetOffsetPtr(table_hdr, val32(table_hdr->rows_offset) + j*val16(table_hdr->row_length), true);

		//DPRINTF("%x\n", DifPointer(row_ptr, table_hdr));
		
		for (uint16_t i = 0; i < val16(table_hdr->num_columns); i++)
		{			
			storage_flag = (columns[i].flags & STORAGE_MASK);	
			
			if (storage_flag == STORAGE_NONE) // 0x00
			{
				current_row.values.push_back(current_value);
				continue;
			}

			if (storage_flag == STORAGE_ZERO) // 0x10
			{
				current_row.values.push_back(current_value);	
				continue;
			}

			if (storage_flag == STORAGE_CONSTANT) // 0x30
			{
				current_row.values.push_back(current_value);				
				continue;
			}
			
			current_value.type = columns[i].flags & TYPE_MASK;
			current_value.position = Utils::DifPointer(row_ptr, table_hdr);

			switch (current_value.type)
			{
				case TYPE_1BYTE: case TYPE_1BYTE2:
					current_value.u1._u8 = row_ptr;
					row_ptr++;
				break;

				case TYPE_2BYTE: case TYPE_2BYTE2:
					current_value.u1._u16 = (uint16_t *)row_ptr;
					row_ptr += 2;
				break;

				case TYPE_4BYTE: case TYPE_4BYTE2:
					current_value.u1._u32 = (uint32_t *)row_ptr;	
					row_ptr += 4;
					//DPRINTF("U32: %x %s\n", val32(*current_row.u1._u64), columns[i].name);
				break;

				case TYPE_8BYTE: case TYPE_8BYTE2:
					current_value.u1._u64 = (uint64_t *)row_ptr;		
					row_ptr += 8;
					//DPRINTF("U64: %I64x %s\n", val64(*current_row.u1._u64), columns[i].name);
				break;

				case TYPE_FLOAT:
					current_value.u1._float = (float *)row_ptr;
					row_ptr += 4;
				break;

				case TYPE_STRING:
					current_value.u1.str = (char *)GetOffsetPtr(strings, *(uint32_t *)row_ptr);	
					row_ptr += 4;
					//DPRINTF("%s -> %s\n", current_row.u1.str, columns[i].name);
				break;

				case TYPE_DATA:
					current_value.u1.data = GetOffsetPtr(data, *(uint32_t *)row_ptr);		
					current_value.data_size = *(uint32_t *)(row_ptr + 4);
					row_ptr += 8;
					current_value.position = Utils::DifPointer(current_value.u1.data, table_hdr);
				break;

				default: 
					DPRINTF("Not implemented!\n");
					return false;
			}
			
			current_row.values.push_back(current_value);			
		}	
		
		rows.push_back(current_row);
		current_row.values.clear();
	}
	
	table_name = (char *)GetOffsetPtr(strings, table_hdr->table_name);	
	return true;
}

void *UtfFile::GetRawData(const char *name, int row)
{
	for (size_t i = 0; i < columns.size(); i++)
	{
		uint32_t storage_flag = (columns[i].flags & STORAGE_MASK);	
		
		if (storage_flag == STORAGE_NONE || storage_flag == STORAGE_ZERO || storage_flag == STORAGE_CONSTANT)
			continue;		
		
		if (strcmp(columns[i].name, name) == 0)
			return rows[row].values[i].GetValue();		
	}
	
	return nullptr;
}

bool UtfFile::GetInteger(const char *name, uint64_t *ret, int row)
{
	for (size_t i = 0; i < columns.size(); i++)
	{
		uint32_t storage_flag = (columns[i].flags & STORAGE_MASK);	
			
		if (storage_flag == STORAGE_NONE || storage_flag == STORAGE_CONSTANT)
		{
			continue;
		}
		
		if (strcmp(columns[i].name, name) == 0)
		{
			if (storage_flag == STORAGE_ZERO)
			{
				*ret = 0;
				return true;
			}
			
			//void *obj = utf.rows[row].rows[i].GetValue();
			int type = rows[row].values[i].type;
			
			switch (type)
			{
				case TYPE_1BYTE: case TYPE_1BYTE2:
					*ret = (uint64_t) *rows[row].values[i].u1._u8;
					return true;
				break;
				
				case TYPE_2BYTE: case TYPE_2BYTE2:
					*ret = (uint64_t) val16(*rows[row].values[i].u1._u16);
					return true;
				break;
				
				case TYPE_4BYTE: case TYPE_4BYTE2:
					*ret = (uint64_t) val32(*rows[row].values[i].u1._u32);
					return true;
				break;
				
				case TYPE_8BYTE: case TYPE_8BYTE2:
					*ret = val64(*rows[row].values[i].u1._u64);
					return true;
				break;
				
				default:
					DPRINTF("%s: %s is not integer.\n", FUNCNAME, name);
					return false;
			}
		}
	}
	
	return false;
}

uint32_t UtfFile::GetPosition(const char *name, int row)
{
	for (size_t i = 0; i < columns.size(); i++)
	{
		uint32_t storage_flag = (columns[i].flags & STORAGE_MASK);	
			
		if (storage_flag == STORAGE_NONE || storage_flag == STORAGE_ZERO || storage_flag == STORAGE_CONSTANT)
		{
			continue;
		}
		
		if (strcmp(columns[i].name, name) == 0)
		{
			return rows[row].values[i].position;			
		}
	}
	
	return (uint32_t)-1;
}

bool UtfFile::SetInteger(const char *name, uint64_t value, int row)
{
	for (size_t i = 0; i < columns.size(); i++)
	{
		uint32_t storage_flag = (columns[i].flags & STORAGE_MASK);	
			
		if (storage_flag == STORAGE_NONE || storage_flag == STORAGE_CONSTANT || storage_flag == STORAGE_ZERO)
		{
			continue;
		}
		
		if (strcmp(columns[i].name, name) == 0)
		{
			int type = rows[row].values[i].type;
            uint16_t v16;
            uint32_t v32;
			
			switch (type)
			{
				case TYPE_1BYTE: case TYPE_1BYTE2:
					
					*rows[row].values[i].u1._u8 = (value&0xFF);
					return true;
					
				break;
				
				case TYPE_2BYTE: case TYPE_2BYTE2:
					
					v16 = value&0xFFFF;
					*rows[row].values[i].u1._u16 = val16(v16);
					return true;
					
				break;
				
				case TYPE_4BYTE: case TYPE_4BYTE2:
					
					v32 = value&0xFFFFFFFF;
					*rows[row].values[i].u1._u32 = val32(v32);
					return true;
					
				break;
				
				case TYPE_8BYTE: case TYPE_8BYTE2:
					
					*rows[row].values[i].u1._u64 = value;
					return true;
					
				break;
				
				default:
					DPRINTF("%s: %s is not integer.\n", FUNCNAME, name);
					return false;
			}
		}
	}
	
	return false;
}

