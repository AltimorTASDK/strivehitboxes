#pragma once

#include "bitmask.h"
#include <map>
#include <functional>
#include <vector>
#include <array>
#include <utility>

namespace bbscript {

extern const short *instruction_sizes;

enum class opcode : int {
	begin_state = 0,
	end_state = 1,
	set_sprite = 2,
	goto_label = 12,
	goto_if_operation = 13,
	call_subroutine = 17,
	upon = 21,
	end_upon = 22,
	remove_upon = 23,
	goto_if = 24,
	set_event_trigger = 36,
	call_subroutine_args = 75,
	modify_run_momentum = 143,
	set_speed_mult = 207,
	MAX = 0xA24
};

enum class variable_type : int {
	abs_distance_x = 0xE,
	abs_distance_y = 0xF,
	pushbox_distance = 0x6A
};

enum class event_type : int {
	immediate = 0,
	before_exit = 1,
	frame_step = 3,
	fall_speed = 4, // requires trigger value
	hit_or_guard = 9,
	hit = 10,
	timer = 13, // requires trigger value
	guard = 74,
	MAX = 0x66
};

using event_bitmask = bitmask<(size_t)event_type::MAX>;

enum class value_type : int {
	constant = 0,
	variable = 2
};

enum class operation : int {
	add = 0,
	sub = 1,
	mul = 2,
	div = 3,
	mod = 4,
	bool_and = 5,
	bool_or = 6,
	bit_and = 7,
	bit_or = 8,
	eq = 9,
	lt = 10,
	gt = 11,
	ge = 12,
	le = 13,
	not_and = 14, // ~a & b
	ne = 15,
	mod_0 = 16, // b % a == 0
	mod_1 = 17, // b % a == 1
	mod_2 = 18, // b % a == 2
	mul_direction_offset = 19, // a * direction + b
	select_a = 20, // a
	mul_direction = 21, // a * direction
	unk22 = 22
};

struct value {
	value_type type;
	union {
		int constant;
		variable_type variable;
	};
};

// Instruction encodings, excluding opcode
template<opcode op>
struct instruction;

template<>
struct instruction<opcode::begin_state> {
	char state[32];
};

template<>
struct instruction<opcode::end_state> {
};

template<>
struct instruction<opcode::set_sprite> {
	char sprite[32];
	int frames;
};

template<>
struct instruction<opcode::goto_label> {
	char label[32];
};

template<>
struct instruction<opcode::goto_if_operation> {
	char label[32];
	operation op;
	value a;
	value b;
};

template<>
struct instruction<opcode::call_subroutine> {
	char subroutine[32];
};

template<>
struct instruction<opcode::upon> {
	event_type event;
};

template<>
struct instruction<opcode::end_upon> {
};

template<>
struct instruction<opcode::remove_upon> {
	event_type event;
};

template<>
struct instruction<opcode::goto_if> {
	char label[32];
	value value;
};

template<>
struct instruction<opcode::set_event_trigger> {
	event_type event;
	// Event specific (e.g. timer)
	int value;
};

template<>
struct instruction<opcode::call_subroutine_args> {
	char subroutine[32];
	value args[4];
};

template<>
struct instruction<opcode::set_speed_mult> {
	int mult;
};

class code_pointer {
	const char *ptr;

public:
	opcode next_op() const
	{
		return *(opcode*)ptr;
	}

	template<opcode op>
	const instruction<op> &peek() const
	{
		return *(instruction<op>*)(ptr + sizeof(opcode));
	}

	template<opcode op>
	const instruction<op> &read()
	{
		const auto &result = *(instruction<op>*)(ptr + sizeof(opcode));
		ptr += sizeof(opcode) + sizeof(result);
		return result;
	}

	const void *read()
	{
		const void *result = (const void*)(ptr + sizeof(opcode));
		skip();
		return result;
	}

	void skip()
	{
		ptr += instruction_sizes[(size_t)next_op()];
	}
};

// Used to allow handling instructions by implementing handle_instruction(const instruction<opcode>&).
// Instruction handlers must be forward declared so their presence is known as compile time.
//
// Implementing classes must create a base class inheriting from instruction_dispatcher_base, and then
// provide a type alias to instruction_dispatcher<base_class>.
class instruction_dispatcher_base {
protected:
	virtual void dispatch_instruction(opcode op, const void *inst) = 0;
};

template<typename T>
class instruction_dispatcher : public T {
public:
	template<typename ...args_type>
	instruction_dispatcher(args_type &&...args) : T(std::forward<args_type>(args)...)
	{
	}

private:
	using opcode_indices = std::make_index_sequence<(size_t)opcode::MAX>;

	template<size_t ...I>
	static constexpr auto create_handler_array_impl(std::index_sequence<I...>)
	{
		return std::array { &handle_instruction_void<(opcode)I>... };
	}

	template<typename indices = std::make_index_sequence<(size_t)opcode::MAX>>
	static constexpr auto create_handler_array()
	{
		return create_handler_array_impl(indices());
	}

	template<opcode op, typename enable = void>
	struct is_implemented { static constexpr auto value = false; };

	template<opcode op>
	struct is_implemented<op, decltype(std::declval<T>().handle_instruction(std::declval<instruction<op>>()))> {
		static constexpr auto value = true;
	};

	template<opcode op>
	void handle_instruction_void(const void *inst)
	{
		if constexpr (is_implemented<op>::value)
			T::handle_instruction(*(const instruction<op>*)inst);
	}

	static constexpr auto handler_array = create_handler_array();

protected:
	void dispatch_instruction(opcode op, const void *inst) override
	{
		return (this->*handler_array[(size_t)op])(inst);
	}
};

class context_base : public instruction_dispatcher_base {
protected:
	code_pointer ip;
	event_bitmask event_handler_bitmask;
	std::vector<std::array<int, 4>> arg_stack;

	struct {
		code_pointer script;
		int trigger_value;
	} event_handlers[(size_t)event_type::MAX];

public:
	context_base(code_pointer ip) : ip(ip)
	{
	}

protected:
	void handle_instruction(const instruction<opcode::begin_state> &inst);
};

using context = instruction_dispatcher<context_base>;

} // bbscript
