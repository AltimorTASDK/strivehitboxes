#include "bbscript.h"
#include "sigscan.h"

const auto *instruction_sizes = (short*)get_rip_relative(
	sigscan::get().scan("\x74\x0D\x48\x83\xC0\x08", "xxxxxx") - 0x15);