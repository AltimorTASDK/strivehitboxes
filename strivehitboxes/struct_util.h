#pragma once

#include <type_traits>

#define FIELD(OFFSET, TYPE, NAME) \
	void set_##OFFSET(std::add_const<TYPE&>::type value) \
	{ \
		*(TYPE*)((char*)this + OFFSET) = value; \
	} \
	\
	void set_##OFFSET(TYPE &&value) \
	{ \
		*(TYPE*)((char*)this + OFFSET) = std::move(value); \
	} \
	\
	TYPE &get_##OFFSET() const \
	{ \
		return *(TYPE*)((char*)this + OFFSET); \
	} \
	__declspec(property(get = get_##OFFSET, put=set_##OFFSET)) TYPE NAME

#define ARRAY_FIELD(OFFSET, TYPE, NAME) \
	TYPE *get_##OFFSET() const \
	{ \
		return (TYPE*)((char*)this + OFFSET); \
	} \
	__declspec(property(get = get_##OFFSET)) TYPE *NAME
