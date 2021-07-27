#include "bbscript.h"
#include "sigscan.h"

namespace bbscript {

const short *instruction_sizes = (short*)get_rip_relative(
	sigscan::get().scan("\x74\x0D\x48\x83\xC0\x08", "xxxxxx") - 0x15);

void context_base::handle_instruction(const instruction<opcode::begin_state> &inst)
{
	*(int*)0xDEADBEEF = 0;
}

} // bbscript