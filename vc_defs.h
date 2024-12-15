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

template <typename T> 
struct StdVector
{
	T *first;
	T *end;
	T *alloc_end;
	
	StdVector()
	{
		first = end = alloc_end = nullptr;
	}
	
	inline size_t Size() const
	{
		return ((uintptr_t)end - (uintptr_t)first) / sizeof(T);
	}
	
	inline size_t Capacity() const
	{
		return ((uintptr_t)alloc_end - (uintptr_t)first) / sizeof(T);
	}
	
	inline void PushBack(T elem, void (* reserve_func)(StdVector<T> *, size_t))
	{
		size_t cap = Capacity();
		
		if (Size() >= cap)
		{
			// Same growth policy than VC
			reserve_func(this, std::max(cap+1, cap+(cap/2)));
		}
		
		*end = elem;
		end++;
	}
	
	inline void Reserve(size_t capacity, void (* reserve_func)(StdVector<T> *, size_t))
	{
		reserve_func(this, capacity);
	}
	
	// Meh no range check
	inline T &operator[](size_t idx)
	{
		return first[idx];
	}
	
	inline const T &operator[](size_t idx) const
	{
		return first[idx];
	}
};
CHECK_STRUCT_SIZE(StdVector<int>, 0x18);

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

