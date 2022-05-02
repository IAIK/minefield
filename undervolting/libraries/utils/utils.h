#pragma once

#include <algorithm>
#include <optional>
#include <stdint.h>
#include <vector>

namespace utils {

bool set_cpu_mask(uint64_t mask);

using bit_histogram_t = std::vector<uint64_t>;

struct Blob {
    std::vector<uint64_t> _data;

    Blob(uint64_t N);
    Blob(uint64_t const *data, uint64_t N);

    Blob operator^(Blob const &other) const;
    Blob operator|(Blob const &other) const;
    Blob operator&(Blob const &other) const;
    Blob operator~() const;

    operator bool() const;

    Blob &operator^=(Blob const &other);
    Blob &operator|=(Blob const &other);
    Blob &operator&=(Blob const &other);

    bool operator==(Blob const &other) const;

    void print(void (*print)(const char *, ...)) const;

    uint8_t hw() const;
    uint8_t hd(Blob const &other) const;

    void accumulate_bit_histogram(bit_histogram_t &bit_histogram) const;
};

void print_bit_histogram(bit_histogram_t const &bit_histogram, void (*print)(const char *, ...));

template<size_t Size>
struct uint64xC : public std::array<uint64_t, Size> {
    using This_t = uint64xC<Size>;

    using std::array<uint64_t, Size>::array;

    constexpr uint64xC() {
        for ( size_t i = 0; i < Size; ++i ) {
            this->operator[](i) = 0;
        }
    }

    constexpr uint64xC(std::initializer_list<uint64_t> list) {
        for ( size_t i = 0; i < Size; ++i ) {
            this->operator[](i) = *(list.begin() + i);
        }
    }

    This_t operator^(This_t const &other) const {
        This_t tmp;
        for ( size_t i = 0; i < Size; ++i ) {
            tmp[i] = this->operator[](i) ^ other[i];
        }
        return tmp;
    }

    This_t operator|(This_t const &other) const {
        This_t tmp;
        for ( size_t i = 0; i < Size; ++i ) {
            tmp[i] = this->operator[](i) | other[i];
        }
        return tmp;
    }

    This_t operator&(This_t const &other) const {
        This_t tmp;
        for ( size_t i = 0; i < Size; ++i ) {
            tmp[i] = this->operator[](i) & other[i];
        }
        return tmp;
    }

    This_t operator~() const {
        This_t tmp;
        for ( size_t i = 0; i < Size; ++i ) {
            tmp[i] = ~this->operator[](i);
        }
        return tmp;
    }

    This_t &operator^=(This_t const &other) {
        *this = *this ^ other;
        return *this;
    }

    This_t &operator|=(This_t const &other) {
        *this = *this | other;
        return *this;
    }

    This_t &operator&=(This_t const &other) {
        *this = *this & other;
        return *this;
    }

    void print(void (*print)(const char *, ...)) {
        print("0x");
        for ( size_t i = 0; i < Size; ++i ) {
            uint64_t val = this->operator[](Size - i - 1);
            print("%016lx", val);
        }
    }

    operator bool() const {
        for ( size_t i = 0; i < Size; ++i ) {
            if ( this->operator[](i) != 0 ) {
                return true;
            }
        }
        return false;
    }

    uint8_t hw() const {
        size_t sum = 0;
        for ( size_t i = 0; i < Size; ++i ) {
            sum += __builtin_popcountl(this->operator[](i));
        }
        return sum;
    }

    uint8_t hd(This_t const &other) const {
        return (*this ^ other).hw();
    }
};

template<typename T>
uint32_t bits32(T x) {
    return *reinterpret_cast<uint32_t *>(&x);
}

template<typename T>
uint64_t bits64(T x) {
    return *reinterpret_cast<uint64_t *>(&x);
}

template<typename Containter_t>
auto median(Containter_t &container) {
    auto b = std::begin(container);
    auto e = std::end(container);
    auto n = std::distance(b, e);
    auto m = b + n / 2;
    std::nth_element(b, m, e);
    return b == e ? std::nullopt : std::optional { *m };
}

template<typename Containter_t>
auto index_reverse(Containter_t &container, size_t index, uint64_t step) {
    return container.data() + container.size() - step - index;
}

uint64_t rdrand64();
uint64_t rdtsc();

} // namespace utils
