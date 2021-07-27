#pragma once

#include "bbscript.h"

namespace bbscript {

class simulated_player_base : public context_base {
public:
	simulated_player_base(code_pointer ip) : context_base(ip)
	{
	}

	using context_base::handle_instruction;
};

using simulated_player = instruction_dispatcher<simulated_player_base>;

} // bbscript
