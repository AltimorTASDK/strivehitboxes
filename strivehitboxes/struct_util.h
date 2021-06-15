#pragma once

#define FIELD(offset, type, name) \
	void set_##offset(type value) \
	{ \
		*(type*)((char*)this + offset) = value; \
	} \
	\
	type &get_##offset() const \
	{ \
		return *(type*)((char*)this + offset); \
	} \
	__declspec(property(get = get_##offset, put=set_##offset)) type name

#define ARRAY_FIELD(offset, type, name) \
	type *get_##offset() const \
	{ \
		return (type*)((char*)this + offset); \
	} \
	__declspec(property(get = get_##offset)) type *name
