#ifndef __UTFLOWLEVEL_H__
#define __UTFLOWLEVEL_H__

#include <stdint.h>
#include <vector>
#include <map>
#include "BaseFile.h"

#define UTF_SIGNATURE	0x46545540

enum COLUMN_FLAGS
{
	STORAGE_MASK = 0xf0,
	STORAGE_NONE = 0x00,
	STORAGE_ZERO = 0x10,
	STORAGE_CONSTANT = 0x30,
	STORAGE_PERROW = 0x50,


	TYPE_MASK = 0x0f,
	TYPE_DATA = 0x0b,
	TYPE_STRING = 0x0a,
	TYPE_FLOAT = 0x08,
	TYPE_8BYTE2 = 0x07,
	TYPE_8BYTE = 0x06,
	TYPE_4BYTE2 = 0x05,
	TYPE_4BYTE = 0x04,
	TYPE_2BYTE2 = 0x03,
	TYPE_2BYTE = 0x02,
	TYPE_1BYTE2 = 0x01,
	TYPE_1BYTE = 0x00,
};

#ifdef _MSC_VER
#pragma pack(push,1)
#endif

typedef struct
{
	uint32_t signature; // 0
	uint32_t table_size; // 4
} PACKED UTFHeader;

STATIC_ASSERT_STRUCT(UTFHeader, 8);

typedef struct
{
	uint32_t rows_offset; // 0
	uint32_t strings_offset; // 4
	uint32_t data_offset;	// 8
	uint32_t table_name; // C
	uint16_t num_columns; // 0x10
	uint16_t row_length; // 0x12
	uint32_t num_rows; // 0x14
} PACKED UTFTableHeader;

STATIC_ASSERT_STRUCT(UTFTableHeader, 0x18);

#ifdef _MSC_VER
#pragma pack(pop)
#endif

struct UtfColumn
{
	uint8_t flags;
	char *name;
};

struct UtfValue
{
	int type;	
	uint32_t data_size; // only for type data	
	uint32_t position;
	
	union
	{
		uint8_t *_u8;
		uint16_t *_u16;
		uint32_t *_u32;
		uint64_t *_u64;
		float *_float;
		char *str;
		uint8_t *data;
	} u1;
	
	UtfValue()
	{
		type = -1;
	}	
	
	void *GetValue();	
	
}; 

struct UtfRow
{
	std::vector<UtfValue> values;	
};

class UtfFile : public BaseFile
{
private:

	UTFTableHeader *table_hdr;
	
	std::vector<UtfColumn> columns;
	std::vector<UtfRow> rows;
	char *table_name;
	
public:

	UtfFile() { big_endian = true; }
	virtual ~UtfFile() {}
	
	virtual bool Load(const uint8_t *buf, size_t size) override;	
	
	inline char *GetTableName() { return table_name; }
	inline uint8_t *GetPtrTable() { return (uint8_t *)table_hdr; }
	inline size_t GetNumColumns() const { return columns.size(); }
	inline size_t GetNumRows() const { return rows.size(); }
	
	void *GetRawData(const char *name, int row=0);	
	bool GetInteger(const char *name, uint64_t *ret, int row=0);
	uint32_t GetPosition(const char *name, int row=0);
	
	bool SetInteger(const char *name, uint64_t value, int row=0);		
};

#endif /* __UTFLOWLEVEL_H__ */
