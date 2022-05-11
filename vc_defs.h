#pragma once

#define BUF_SIZE_STR	16
#define BUF_SIZE_U16STR	8

struct StdString
{	
	union
	{
		char buf[BUF_SIZE_STR]; // For allocation_size < BUF_SIZE_STR
		char *ptr; // For allocation_size >= BUF_SIZE_STR		
	} str; 
	
	size_t length; 
	size_t allocation_size; 
	
	StdString()
	{
		str.ptr = nullptr;
		length = 0;
		allocation_size = 0;
	}
	
	inline const char *CStr() const
	{
		if (allocation_size >= BUF_SIZE_STR)
		{
			return str.ptr;
		}
		
		return str.buf;
	}
};
CHECK_STRUCT_SIZE(StdString, 0x20);

struct StdU16String
{
	union
	{
		char16_t buf[BUF_SIZE_U16STR]; // For allocation_size < BUF_SIZE_U16STR
		char16_t *ptr; // For allocation_size >= BUF_SIZE_U16STR
	} str;
	
	size_t length;
	size_t allocation_size;
	
	StdU16String()
	{
		str.ptr = nullptr;
		length = 0;
		allocation_size = 0;
	}
	
	inline const char16_t *CStr() const
	{
		if (allocation_size >= BUF_SIZE_U16STR)
		{
			return str.ptr;
		}
		
		return str.buf;
	}
};
CHECK_STRUCT_SIZE(StdU16String, 0x20);

