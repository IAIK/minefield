#include "logging.h"
#include "undervolting.h"
#include "utils.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <iterator>
#include <numeric>
#include <optional>
#include <string_view>
#include <unistd.h>
#include <vector>

using namespace utils;

struct i128 {
    uint64_t l;
    uint64_t h;
};

using target_instruction_t = i128 (*)(uint64_t const *init, uint64_t init_count, uint64_t *results,
                                      uint64_t results_count, uint64_t trap_factor, uint64_t trap_init);
using get_count_t          = uint64_t (*)();
using InitState_t          = std::array<uint64_t, 8>;

uint64_t analyse_instruction(std::vector<uint64_t> const &reference, uint64_t step, bool verbose) {

    bool is_steady = false;

    Blob init(index_reverse(reference, 0, step), step);
    Blob state = init;

    Blob mask(step);

    uint64_t periode = 0;

    std::vector<uint8_t> bit_changes(reference.size() / step);

    for ( uint64_t i = step; i < reference.size(); i += step ) {
        Blob new_state(index_reverse(reference, i, step), step);

        // check if the value didn't change
        if ( new_state == state ) {
            is_steady = true;
        }

        // calculate periode
        if ( new_state == init && periode == 0 ) {
            periode = i / step;
        }

        // state mask

        /*new_state.print();
        printf(" vs ");
        state.print();
        printf("\n");*/

        mask |= state ^ new_state;

        bit_changes[i / step] = state.hd(new_state);

        state = new_state;
        /*if ( i < 5 ) {
            state.print();
            logging::write("\n");
        }*/
    }

    uint8_t median_bit_changes = median(bit_changes).value_or(-1);

    if ( verbose ) {
        logging::write("\ninstruction:\n"
                       "  steady:        %16d\n"
                       "  periode:       %16lu\n"
                       "  bit changes:   %16d\n"
                       "  mask:        ",
                       is_steady, periode, median_bit_changes);
        mask.print(logging::write);
        logging::write("\n\n");
    }

    return median_bit_changes;
}

struct FaultingStats {
    uint64_t              faulted_iterations;
    uint64_t              median_bit_changes;
    Blob                  fault_mask;
    std::vector<uint64_t> to_1_flips;
    std::vector<uint64_t> to_0_flips;
    std::vector<uint64_t> fault_histogram;
    uint64_t              detected;
    uint64_t              time_ticks;
    double                base_voltage;

    undervolting::OperatingPoint min;
    undervolting::OperatingPoint max;
    undervolting::OperatingPoint avg;
    int                          op_count = 0;

    FaultingStats(uint64_t N, uint64_t iterations)
      : faulted_iterations { 0 }
      , median_bit_changes { 0 }
      , fault_mask { N }
      , detected { 0 }
      , time_ticks { 0 }
      , base_voltage { 0 } {
        to_1_flips.resize(N * 64);
        to_0_flips.resize(N * 64);
        fault_histogram.resize(iterations);
    }

    void add_operating_point(undervolting::OperatingPoint const &op) {
        if ( op_count++ == 0 ) {
            min = op;
            max = op;
            avg = op;
        }
        else {
            min.frequency      = std::min(min.frequency, op.frequency);
            min.voltage        = std::min(min.voltage, op.voltage);
            min.plane_0_offset = std::min(min.plane_0_offset, op.plane_0_offset);
            min.plane_2_offset = std::min(min.plane_2_offset, op.plane_2_offset);

            max.frequency      = std::max(max.frequency, op.frequency);
            max.voltage        = std::max(max.voltage, op.voltage);
            max.plane_0_offset = std::max(max.plane_0_offset, op.plane_0_offset);
            max.plane_2_offset = std::max(max.plane_2_offset, op.plane_2_offset);

            avg.frequency += op.frequency;
            avg.voltage += op.voltage;
            avg.plane_0_offset += op.plane_0_offset;
            avg.plane_2_offset += op.plane_2_offset;
        }
    }

    void finalize() {
        avg.frequency      = avg.frequency / op_count;
        avg.voltage        = avg.voltage / op_count;
        avg.plane_0_offset = avg.plane_0_offset / op_count;
        avg.plane_2_offset = avg.plane_2_offset / op_count;
    }

    void print_faulting_hist() {
        for ( uint64_t i = 0; i < fault_histogram.size(); ++i ) {
            if ( fault_histogram[i] ) {
                printf("%llu=%llu,", i, fault_histogram[i]);
            }
        }
    }
};

void analyse_results(FaultingStats &stats, std::vector<uint64_t> const &reference, std::vector<uint64_t> const &results,
                     uint64_t store_count, bool verbose) {

    for ( uint64_t i = 0; i < reference.size(); i += store_count ) {

        Blob const res(index_reverse(results, i, store_count), store_count);
        Blob const ref(index_reverse(reference, i, store_count), store_count);
        Blob const mask = res ^ ref;

        if ( mask ) {
            stats.fault_mask |= mask;
            stats.faulted_iterations += 1;

            Blob const to_1_flips = res & mask;    // has ones by 0 -> 1 flips
            Blob const to_0_flips = (~res) & mask; // has ones by 1 -> 0 flips

            to_1_flips.accumulate_bit_histogram(stats.to_1_flips);
            to_0_flips.accumulate_bit_histogram(stats.to_0_flips);
            stats.fault_histogram[i / store_count] += 1;
        }
    }

    if ( verbose ) {
        logging::write("%10ld; ", (uint64_t)stats.faulted_iterations);
        stats.fault_mask.print(logging::write);
        logging::write(";");
        print_bit_histogram(stats.to_1_flips, logging::write);
        logging::write(";");
        print_bit_histogram(stats.to_0_flips, logging::write);
        // logging::write(new_faults ? "\n" : "\r");
        logging::write("\n");
    }
}

FaultingStats analyse_inst(uint64_t rounds, uint64_t iterations, target_instruction_t assembly, uint64_t load_count,
                           uint64_t store_count, bool verbose, uint64_t trap_factor, uint64_t trap_init) {

    std::vector<uint64_t> inits(iterations * load_count);
    std::vector<uint64_t> reference(iterations * store_count);
    std::vector<uint64_t> results(iterations * store_count);

    // get the init states for each iteration
    for ( uint64_t &x : inits ) {
        x = rdrand64();
    }

    // record reference
    i128 ref_mark = assembly(inits.data(), inits.size(), reference.data(), reference.size(), trap_factor, trap_init);

    // printf("ref_mark 0x%16lx%16lx\n", ref_mark.h, ref_mark.l);

    FaultingStats stats(store_count, iterations);

    for ( uint64_t i = 0; i < 100; ++i ) {
        undervolting::OperatingPoint op = undervolting::get_operating_point();
        usleep(1000);
        stats.base_voltage += op.voltage;
    }
    stats.base_voltage /= 100;

    // analyse reference
    stats.median_bit_changes = analyse_instruction(reference, store_count, verbose);

    // header
    if ( verbose )
        logging::write("%5s; %10s;", "round", "count");
    {
        char     buffer[100];
        uint32_t spacing = store_count * 17 + 2;
        snprintf(buffer, 100, " %%%ds; %%%ds; %%%ds\n", spacing, spacing, spacing);
        if ( verbose )
            logging::write(buffer, "fault mask", "to 1 faults", "to 0 faults");
    }

    // run
    for ( uint64_t round = 0; round < rounds; ++round ) {
        if ( verbose )
            logging::write("%5ld; ", round);

        undervolting::begin();
        uint64_t tsc_start = rdtsc();

        i128     mark    = assembly(inits.data(), inits.size(), results.data(), results.size(), trap_factor, trap_init);
        uint64_t tsc_end = rdtsc();

        undervolting::OperatingPoint op = undervolting::get_operating_point();

        undervolting::end();

        uint64_t fault_detected = mark.h != ref_mark.h || mark.l != ref_mark.l;
        // printf("mark 0x%16lx%16lx\n", mark.h, mark.l);
        stats.time_ticks = (tsc_end - tsc_start);
        stats.add_operating_point(op);

        stats.detected += fault_detected;

        analyse_results(stats, reference, results, store_count, verbose);

        // clear results
        std::memset(results.data(), 0, results.size() * sizeof(uint64_t));
    }

    return stats;
}

// FaultingStats stressor_stats;

void *stressor(void *data) {
    uint8_t core = (uint8_t)(reinterpret_cast<uint64_t>(data) >> 0) & 0xFF;
    uint8_t ht   = (uint8_t)(reinterpret_cast<uint64_t>(data) >> 8) & 0xFF;

    if ( !set_cpu_mask(1 << core) ) {
        exit(-1);
    }

    while ( true ) {
        uint64_t init[4] = { -1llu, -1llu, -1llu, -1llu };

        asm volatile("    vlddqu %[init], %%xmm4        \n"
                     "    vlddqu %[init], %%xmm5        \n"
                     "    vpxor %%xmm3, %%xmm3, %%xmm3  \n"
                     "    mov $100000000, %%rdx         \n"
                     "1:                                \n"
                     "    push %%r10                    \n"
                     "    vpsllq %%xmm3, %%xmm4, %%xmm6 \n"
                     "    vpsllq %%xmm3, %%xmm5, %%xmm7 \n"
                     "    pop %%r10                     \n"
                     "    dec %%rdx                     \n"
                     "    jnz 1b                        \n"
                     :
                     : [ init ] "m"(init)
                     : "rdx", "xmm4", "xmm5", "xmm3", "xmm6", "xmm7");

        if ( ht ) {
            // if we run on the hyperthread measure the pstate and the core volatage
            // undervolting::OperatingPoint op = undervolting::get_operating_point();
            // stressor_stats.add_operating_point(op);
        }
    }

    return nullptr;
}

uint64_t convert_int(char *string) {
    if ( string[0] == '0' && string[1] == 'x' ) {
        return strtoul(string, NULL, 16);
    }
    else {
        return strtoul(string, NULL, 10);
    }
}

int main(int argc, char *argv[]) {
    if ( argc != 12 ) {
        printf("usage: %s file frequency voltage_offset target_core sibling_core core_count rounds iterations "
               "verbose trap_factor trap_init\n",
               argv[0]);
        return -1;
    }

    char const *   file            = argv[1];
    uint64_t const frequency       = strtol(argv[2], NULL, 10);
    int64_t const  voltage_offset  = strtol(argv[3], NULL, 10);
    uint8_t const  target_core     = static_cast<uint8_t>(strtoul(argv[4], NULL, 10));
    uint8_t const  sibling_core    = static_cast<uint8_t>(strtoul(argv[5], NULL, 10));
    uint8_t const  core_count      = static_cast<uint8_t>(strtoul(argv[6], NULL, 10));
    uint64_t const rounds          = strtoul(argv[7], NULL, 10);
    uint64_t const iterations      = strtoul(argv[8], NULL, 10); // 1000 * 9600;
    bool           verbose         = strtoul(argv[9], NULL, 10);
    uint64_t       trap_factor     = convert_int(argv[10]);
    uint64_t       trap_init       = convert_int(argv[11]);
    uint64_t       show_fault_hist = false; // strtoul(argv[12], NULL, 10);

    char const *instruction_name = file;

    {
        char const *cur = file;
        while ( *cur != 0 ) {
            if ( *cur == '/' ) {
                instruction_name = cur + 1;
            }
            cur++;
        }
    }

    std::vector<pthread_t> threads;

    // pin to target core
    if ( !set_cpu_mask(1 << target_core) ) {
        return -2;
    }

    // create stressors on the other cores
    for ( uint8_t core = 0; core < core_count; ++core ) {
        if ( core == target_core )
            continue;
        threads.push_back(0);

        uint64_t data = core | ((core == sibling_core) << 8);

        pthread_create(&threads.back(), nullptr, stressor, reinterpret_cast<void *>(data));
    }

    // try to connect to the logging server
    // logging::open(ip, 65431);

    undervolting::open(target_core);
    undervolting::set_undervolting_target(voltage_offset);

    if ( verbose )
        logging::write("running: %s\n", file);

    void *libtarget = dlopen(file, RTLD_LAZY);

    if ( !libtarget ) {
        logging::write("could not load %s\n", file);
        return -1;
    }

    target_instruction_t target_instruction = (target_instruction_t)dlsym(libtarget, "target_instruction_protected");
    get_count_t          get_load_count     = (get_count_t)dlsym(libtarget, "get_load_count");
    get_count_t          get_store_count    = (get_count_t)dlsym(libtarget, "get_store_count");

    // if ( !verbose ) {
    //    std::string file_name(file);
    //    std::string instruction_name = file_name.substr(file_name.find_last_of("/") + 1);
    //    logging::write("%4d %3d %2d %-50s", frequency, voltage_offset, target_core, instruction_name.c_str());
    // }
    logging::write("%4llu; %4lld; %2u; %3u; %6u; %-30s; ", frequency, voltage_offset, target_core, rounds, iterations,
                   instruction_name);

    FaultingStats stats = analyse_inst(rounds, iterations, target_instruction, get_load_count(), get_store_count(),
                                       verbose, trap_factor, trap_init);

    stats.finalize();

    if ( !verbose ) {
        logging::write("faults=%4ld; detected=%4ld; ", stats.faulted_iterations, stats.detected);

        logging::write("f=[%4.1f,%4.1f,%4.1f]; v=[%7.4f,%7.4f,%7.4f]; v_base=%7.4f; vo=%7.4f; factor=0x%llx; "
                       "init=0x%llx; bc=%3ld; "
                       "temp=%u; ticks=%llu; mask=",
                       stats.min.frequency, stats.avg.frequency, stats.max.frequency, stats.min.voltage,
                       stats.avg.voltage, stats.max.voltage, stats.base_voltage, stats.avg.plane_0_offset, trap_factor,
                       trap_init, stats.median_bit_changes, undervolting::get_temperature(), stats.time_ticks);
        /*
                logging::write("pstate=[%4lld,%4lld]; vabs=[%7.4f;%7.4f;%7.4f]; n=%3llu; mask=",
           (int64_t)pstate_min, (int64_t)pstate_max, (int64_t)voltage_min / 8192.0,
                               ((int64_t)voltage_avg / 8192.0) / operating_points_measured, (int64_t)voltage_max /
           8192.0, operating_points_measured);
        */
        stats.fault_mask.print(logging::write);
        logging::write("; to_1={");
        print_bit_histogram(stats.to_1_flips, logging::write);
        logging::write("}; to_0={");
        print_bit_histogram(stats.to_0_flips, logging::write);
        logging::write("}");

        if ( show_fault_hist ) {
            logging::write("; hist={");
            stats.print_faulting_hist();
            logging::write("}");
        }
        logging::write("\n");
    }

    dlclose(libtarget);

    // cleanup
    undervolting::close();
    logging::close();

    return stats.faulted_iterations > 0 ? 3 : 0;
}
