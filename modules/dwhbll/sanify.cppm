module;

#include <dwhbll/sanify/deferred.h>

export module dwhbll.sanify;

import std;

export using i8 = std::int8_t;
export using i16 = std::int16_t;
export using i32 = std::int32_t;
export using i64 = std::int64_t;

export using u8 = std::uint8_t;
export using u16 = std::uint16_t;
export using u32 = std::uint32_t;
export using u64 = std::uint64_t;

export using f32 = float;
export using f64 = double;

export namespace dwhbll::sanify {
    using dwhbll::sanify::deferred;
}
