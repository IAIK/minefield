#pragma once

#include <stdint.h>

namespace fault_hardening {

uint64_t get_fault_count();
uint64_t get_fault_mask();

} // namespace fault_hardening