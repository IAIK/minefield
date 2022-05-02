#pragma once

#include <stdint.h>

#define UNDERVOLTING_ASM_BEGIN
#define UNDERVOLTING_BEGIN

namespace undervolting {

void open(uint8_t core);
void close();

void set_undervolting_target(uint64_t offset);

void begin();
void end();

uint8_t get_temperature();

double get_voltage();

struct OperatingPoint {
    double frequency;
    double voltage;
    double plane_0_offset;
    double plane_2_offset;
};

OperatingPoint get_operating_point();

} // namespace undervolting
