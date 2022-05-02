#include "utils.h"

#include "immintrin.h"

#define _GNU_SOURCE
#include <cassert>
#include <pthread.h>
#include <vector>

namespace utils {

bool set_cpu_mask(uint64_t mask) {
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
    return pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset) == 0;
}

Blob::Blob(uint64_t N) {
    for ( uint64_t i = 0; i < N; ++i ) {
        _data.push_back(0);
    }
}

Blob::Blob(uint64_t const *data, uint64_t N) {
    for ( uint64_t i = 0; i < N; ++i ) {
        _data.push_back(data[i]);
    }
}

Blob Blob::operator^(Blob const &other) const {
    assert(_data.size() == other._data.size());

    Blob tmp(_data.data(), _data.size());

    for ( uint64_t i = 0; i < _data.size(); ++i ) {
        tmp._data[i] ^= other._data[i];
    }
    return tmp;
}

Blob Blob::operator|(Blob const &other) const {
    assert(_data.size() == other._data.size());

    Blob tmp(_data.data(), _data.size());

    for ( uint64_t i = 0; i < _data.size(); ++i ) {
        tmp._data[i] |= other._data[i];
    }
    return tmp;
}

Blob Blob::operator&(Blob const &other) const {
    assert(_data.size() == other._data.size());

    Blob tmp(_data.data(), _data.size());

    for ( uint64_t i = 0; i < _data.size(); ++i ) {
        tmp._data[i] &= other._data[i];
    }
    return tmp;
}

Blob Blob::operator~() const {
    Blob tmp(_data.data(), _data.size());
    for ( uint64_t i = 0; i < _data.size(); ++i ) {
        tmp._data[i] = ~tmp._data[i];
    }
    return tmp;
}

Blob &Blob::operator^=(Blob const &other) {
    *this = *this ^ other;
    return *this;
}

Blob &Blob::operator|=(Blob const &other) {
    *this = *this | other;
    return *this;
}

Blob &Blob::operator&=(Blob const &other) {
    *this = *this & other;
    return *this;
}

bool Blob::operator==(Blob const &other) const {
    return _data.size() == other._data.size() && _data == other._data;
}

void Blob::print(void (*print)(const char *, ...)) const {
    print("0x");
    for ( size_t i = 0; i < _data.size(); ++i ) {
        print("_%016lx", _data[_data.size() - i - 1]);
    }
}

Blob::operator bool() const {
    for ( auto const &x : _data ) {
        if ( x != 0 ) {
            return true;
        }
    }
    return false;
}

uint8_t Blob::hw() const {
    size_t sum = 0;
    for ( auto const &x : _data ) {
        sum += __builtin_popcountl(x);
    }
    return (uint8_t)sum;
}

uint8_t Blob::hd(Blob const &other) const {
    return (*this ^ other).hw();
}

void Blob::accumulate_bit_histogram(std::vector<uint64_t> &bit_histogram) const {
    assert(bit_histogram.size() == _data.size() * 64);
    for ( int k = 0; k < _data.size(); ++k ) {
        for ( int i = 0; i < 64; ++i ) {
            bit_histogram[k * 64 + i] += (_data[k] >> i) & 1;
        }
    }
}

void print_bit_histogram(bit_histogram_t const &bit_histogram, void (*print)(const char *, ...)) {
    bool none = true;
    for ( size_t i = 0; i < bit_histogram.size(); ++i ) {
        if ( bit_histogram[i] ) {
            print("b%d=%d,", i, bit_histogram[i]);
            none = false;
        }
    }
    if ( none ) {
        print("");
    }
}

uint64_t rdrand64() {
    unsigned long long wtf;
    _rdrand64_step(&wtf);
    return static_cast<uint64_t>(wtf);
}

uint64_t rdtsc() {
    uint64_t a, d;
    asm volatile("mfence");
    asm volatile("rdtsc" : "=a"(a), "=d"(d));
    a = (d << 32) | a;
    asm volatile("mfence");
    return a;
}

} // namespace utils