#include "logging.h"
#include "undervolting.h"

#include <iterator>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

uint64_t scale = 0;

void set_cpu_mask(int mask) {
    pthread_t thread = pthread_self();
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    int j = 0;
    while ( mask ) {
        if ( mask & 0x1 ) {
            CPU_SET(j, &cpuset);
        }
        mask >>= 1;
        j++;
    }

    int status = pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset);
    if ( status != 0 ) {
        printf("error while setting cpu mask!\n");
    }
}

uint64_t rdtsc() {
    uint64_t a, d;
    asm volatile("mfence");
    asm volatile("rdtsc" : "=a"(a), "=d"(d));
    a = (d << 32) | a;
    asm volatile("mfence");
    return a;
}

double threshold_voltage_down;
double threshold_voltage_up;

uint64_t      start, end;
uint64_t      delay_ticks;
int           rounds;
volatile bool go       = false;
volatile bool finished = false;
volatile bool init     = false;

void *measurement_thread(void *) {
    set_cpu_mask(1 << 0);

    while ( !init ) {
    }

    puts("measurement_thread");

    for ( int i = 0; i < rounds; ++i ) {
        fprintf(stderr, "%d,", i);
        go = true;
        while ( undervolting::get_voltage() > threshold_voltage_down && !finished ) {
        }
        start = rdtsc();

        while ( undervolting::get_voltage() < threshold_voltage_up && !finished ) {
        }
        bool aborted = finished;
        end          = rdtsc();

        uint64_t diff = end - start;
        if ( !aborted )
            fprintf(stderr, "%f\n", diff * 1000000.0 / (double)scale); // convert to us
        else
            fprintf(stderr, "0\n");

        while ( !finished )
            ;
        finished = false;
        usleep(500); // to make wrong misses visible
    }
    return NULL;
}

void *undervolting_thread(void *) {

    set_cpu_mask(1 << 1);

    puts("undervolting_thread");
    init = true;

    while ( true ) {
        while ( !go ) {
        }
        go           = false;
        uint64_t end = delay_ticks + rdtsc();
        undervolting::begin();
        while ( rdtsc() < end ) {
        }
        undervolting::end();
        usleep(10000);
        finished = true;
    }
    return NULL;
}

extern "C" int main(int argc, char **argv) {
    srand(0xdeadbeaf);

    uint64_t start = rdtsc();
    sleep(1);
    uint64_t end = rdtsc();
    scale        = (end - start);

    printf("scale %lu\n", scale);

    /*uint64_t start = rdtsc();
    sleep(10);
    uint64_t end = rdtsc();
    printf("diff: %lu\n", end - start);
    return 0;*/

    if ( argc != 5 ) {
        printf("%s rounds offset core delay\n", argv[0]);
        exit(-1);
    }

    rounds         = strtol(argv[1], NULL, 10);
    int64_t offset = strtol(argv[2], NULL, 10);
    uint8_t core   = strtol(argv[3], NULL, 10);
    delay_ticks    = strtol(argv[4], NULL, 10);

    undervolting::open(core);

    undervolting::set_undervolting_target(0);

    undervolting::begin();

    double voltage         = undervolting::get_voltage();
    threshold_voltage_down = voltage + ((offset / 2000.0) * 4.0 / 5.0);
    threshold_voltage_up   = voltage + ((offset / 2000.0) * 1.0 / 5.0);

    printf("using thresholds %f-%f\n", threshold_voltage_down, threshold_voltage_up);

    undervolting::set_undervolting_target(offset);

    pthread_t t;
    pthread_create(&t, NULL, undervolting_thread, NULL);

    measurement_thread(NULL);
    undervolting::end();
    undervolting::close();

    return 0;
}
