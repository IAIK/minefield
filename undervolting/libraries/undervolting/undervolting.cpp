// adapted from the plundervolt PoC

#include "undervolting.h"

#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace undervolting {

static int      fd;
static uint64_t plane0_offset, plane0_return;
static uint64_t plane2_offset, plane2_return;

static void signal_handler(int number) {
    end();
    puts("aborted!");
    exit(-1);
}

static uint64_t calculate_msr_value(int64_t offset, uint64_t plane) {
    uint64_t val;
    val = (offset * 1.024) - 0.5; // -0.5 to deal with rounding issues
    val = 0xFFE00000 & ((val & 0xFFF) << 21);
    val = val | 0x8000001100000000;
    val = val | (plane << 40);
    return (uint64_t)val;
}

static double read_offset(uint64_t plane) {
    uint64_t val = 0x8000001000000000;
    val          = val | (plane << 40);
    pwrite(fd, &val, 8, 0x150);

    uint64_t response;
    pread(fd, &response, 8, 0x150);
    uint64_t plane_index = response >> 40;
    uint64_t x           = response ^ (plane_index << 40);
    uint64_t r           = x >> 21;
    int64_t  raw         = r <= 1024 ? r : -(2048 - r);
    double   offset      = (raw) / 1.024;
    return offset;
}

static void set(uint64_t msr_value) {
    pwrite(fd, &msr_value, 8, 0x150);
}

void open(uint8_t core) {

    char path[20];
    snprintf(path, sizeof(path), "/dev/cpu/%d/msr", core);

    // open the register
    fd = ::open(path, O_RDWR);
    if ( fd == -1 ) {
        printf("UNDERVOLTING: Could not open %s\n", path);
        exit(-1);
    }
    plane0_return = calculate_msr_value(0, 0);
    plane2_return = calculate_msr_value(0, 2);

    signal(SIGINT, signal_handler);
}

void close() {
    if ( fd != -1 ) {
        ::close(fd);
    }
}

void set_undervolting_target(uint64_t offset) {
    plane0_offset = calculate_msr_value(offset, 0);
    plane2_offset = calculate_msr_value(offset, 2);
}

void begin() {
    set(plane0_offset);
    set(plane2_offset);
}

void end() {
    set(plane0_return);
    set(plane2_return);
}

uint8_t get_temperature() {
    uint64_t therm_status, temp_target;
    pread(fd, &therm_status, 8, 0x19c);
    pread(fd, &temp_target, 8, 0x1a2);

    uint8_t dro = (therm_status >> 16) & 0x7F;
    uint8_t tcc = (temp_target >> 16) & 0xFF;
    return tcc - dro;
}

double get_voltage() {
    uint64_t val;
    pread(fd, &val, sizeof(val), 0x198);
    return (double)((val & 0xFFFF00000000) >> 32) / 8192.0;
}

OperatingPoint get_operating_point() {
    uint64_t val;
    pread(fd, &val, sizeof(val), 0x198);
    OperatingPoint x;
    x.frequency      = ((val & 0xFFFF) >> 8) * 100;
    x.voltage        = ((val & 0xFFFF00000000) >> 32) / 8192.0;
    x.plane_0_offset = read_offset(0);
    x.plane_2_offset = read_offset(2);
    return x;
}

} // namespace undervolting
