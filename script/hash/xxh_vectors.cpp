// Generate test vector data
// The file data/hash/xxh_vectors.dat was generated using this program,
// it isn't intended to be regenerated automatically as it depends the user
// having libxxhash installed

// you can build this program using `g++ -std=c++26 xxh_vectors.cpp -lxxhash'

#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "xxhash.h"

void write_u16_le(std::ofstream &ofstream, uint16_t value) {
    if constexpr (std::endian::big == std::endian::native)
        value = std::byteswap(value);
    auto bytes = std::bit_cast<std::array<std::uint8_t, 2>>(value);

    ofstream.write(reinterpret_cast<const std::ostream::char_type *>(bytes.data()), bytes.size());
}

void write_u32_le(std::ofstream &ofstream, uint32_t value) {
    if constexpr (std::endian::big == std::endian::native)
        value = std::byteswap(value);
    auto bytes = std::bit_cast<std::array<std::uint8_t, 4>>(value);

    ofstream.write(reinterpret_cast<const std::ostream::char_type *>(bytes.data()), bytes.size());
}

void write_u64_le(std::ofstream &ofstream, uint64_t value) {
    if constexpr (std::endian::big == std::endian::native)
        value = std::byteswap(value);
    auto bytes = std::bit_cast<std::array<std::uint8_t, 8>>(value);

    ofstream.write(reinterpret_cast<const std::ostream::char_type *>(bytes.data()), bytes.size());
}

void write_len_string(std::ofstream &ofstream, std::string value) {
    write_u16_le(ofstream, value.size());
    ofstream.write(value.data(), value.size());
}

using bytes_rng = std::independent_bits_engine<std::mt19937, 8, std::uint8_t>;

void output_xxh32(std::ofstream& ofstream, uint32_t seed,
    uint8_t *ptr, std::size_t len) {
    XXH32_state_t* const state = XXH32_createState();
    if (!state)
        abort();

    /* Initialize state with selected seed */
    if (XXH32_reset(state, seed) == XXH_ERROR)
        abort();

    write_u16_le(ofstream, len + 1);

    write_u32_le(ofstream, XXH32_digest(state));

    for (std::size_t i = 0; i < len; i++) {
        if (XXH32_update(state, ptr + i, 1) == XXH_ERROR)
            abort();

        write_u32_le(ofstream, XXH32_digest(state));
    }

    XXH32_freeState(state);
}

void output_xxh64(std::ofstream& ofstream, uint64_t seed,
    uint8_t *ptr, std::size_t len) {
    XXH64_state_t* const state = XXH64_createState();
    if (!state)
        abort();

    /* Initialize state with selected seed */
    if (XXH64_reset(state, seed) == XXH_ERROR)
        abort();

    write_u16_le(ofstream, len + 1);

    write_u64_le(ofstream, XXH64_digest(state));

    for (std::size_t i = 0; i < len; i++) {
        if (XXH64_update(state, ptr + i, 1) == XXH_ERROR)
            abort();

        write_u64_le(ofstream, XXH64_digest(state));
    }

    XXH64_freeState(state);
}

void output_xxh3_64seed(std::ofstream& ofstream, uint64_t seed,
    uint8_t *ptr, std::size_t len) {
    XXH3_state_t* const state = XXH3_createState();
    if (!state)
        abort();

    /* Initialize state with selected seed */
    if (XXH3_64bits_reset_withSeed(state, seed) == XXH_ERROR)
        abort();

    write_u16_le(ofstream, len + 1);

    write_u64_le(ofstream, XXH3_64bits_digest(state));

    for (std::size_t i = 0; i < len; i++) {
        if (XXH3_64bits_update(state, ptr + i, 1) == XXH_ERROR)
            abort();

        write_u64_le(ofstream, XXH3_64bits_digest(state));
    }

    XXH3_freeState(state);
}

void output_xxh3_128seed(std::ofstream& ofstream, uint64_t seed,
    uint8_t *ptr, std::size_t len) {
    XXH3_state_t* const state = XXH3_createState();
    if (!state)
        abort();

    /* Initialize state with selected seed */
    if (XXH3_128bits_reset_withSeed(state, seed) == XXH_ERROR)
        abort();

    write_u16_le(ofstream, len + 1);

    auto x = XXH3_128bits_digest(state);
    write_u64_le(ofstream, x.low64);
    write_u64_le(ofstream, x.high64);

    for (std::size_t i = 0; i < len; i++) {
        if (XXH3_128bits_update(state, ptr + i, 1) == XXH_ERROR)
            abort();

        x = XXH3_128bits_digest(state);
        write_u64_le(ofstream, x.low64);
        write_u64_le(ofstream, x.high64);
    }

    XXH3_freeState(state);
}

void output_xxh3_64secret(std::ofstream& ofstream, const std::array<std::uint8_t, 159>& secret,
    uint8_t *ptr, std::size_t len) {
    XXH3_state_t* const state = XXH3_createState();
    if (!state)
        abort();

    if (XXH3_64bits_reset_withSecret(state, secret.data(), secret.size()) == XXH_ERROR)
        abort();

    write_u16_le(ofstream, len + 1);

    write_u64_le(ofstream, XXH3_64bits_digest(state));

    for (std::size_t i = 0; i < len; i++) {
        if (XXH3_64bits_update(state, ptr + i, 1) == XXH_ERROR)
            abort();

        write_u64_le(ofstream, XXH3_64bits_digest(state));
    }

    XXH3_freeState(state);
}

void output_xxh3_128secret(std::ofstream& ofstream, const std::array<std::uint8_t, 159>& secret,
    uint8_t *ptr, std::size_t len) {
    XXH3_state_t* const state = XXH3_createState();
    if (!state)
        abort();

    if (XXH3_128bits_reset_withSecret(state, secret.data(), secret.size()) == XXH_ERROR)
        abort();

    write_u16_le(ofstream, len + 1);

    auto x = XXH3_128bits_digest(state);
    write_u64_le(ofstream, x.low64);
    write_u64_le(ofstream, x.high64);

    for (std::size_t i = 0; i < len; i++) {
        if (XXH3_128bits_update(state, ptr + i, 1) == XXH_ERROR)
            abort();

        x = XXH3_128bits_digest(state);
        write_u64_le(ofstream, x.low64);
        write_u64_le(ofstream, x.high64);
    }

    XXH3_freeState(state);
}

void output_xxh3_64(std::ofstream& ofstream, uint8_t *ptr, std::size_t len) {
    XXH3_state_t* const state = XXH3_createState();
    if (!state)
        abort();

    if (XXH3_64bits_reset(state) == XXH_ERROR)
        abort();

    write_u16_le(ofstream, len + 1);

    write_u64_le(ofstream, XXH3_64bits_digest(state));

    for (std::size_t i = 0; i < len; i++) {
        if (XXH3_64bits_update(state, ptr + i, 1) == XXH_ERROR)
            abort();

        write_u64_le(ofstream, XXH3_64bits_digest(state));
    }

    XXH3_freeState(state);
}

void output_xxh3_128(std::ofstream& ofstream, uint8_t *ptr, std::size_t len) {
    XXH3_state_t* const state = XXH3_createState();
    if (!state)
        abort();

    if (XXH3_128bits_reset(state) == XXH_ERROR)
        abort();

    write_u16_le(ofstream, len + 1);

    auto x = XXH3_128bits_digest(state);
    write_u64_le(ofstream, x.low64);
    write_u64_le(ofstream, x.high64);

    for (std::size_t i = 0; i < len; i++) {
        if (XXH3_128bits_update(state, ptr + i, 1) == XXH_ERROR)
            abort();

        x = XXH3_128bits_digest(state);
        write_u64_le(ofstream, x.low64);
        write_u64_le(ofstream, x.high64);
    }

    XXH3_freeState(state);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: xxh_vectors <data block> <output path>" << std::endl;
        return 1;
    }

    std::filesystem::path src_file(argv[1]);
    std::filesystem::path dst_file(argv[2]);

    if (!std::filesystem::exists(src_file)) {
        std::cerr << "The source file to be hashed doesn't exist!" << std::endl;
        return 1;
    }

    bytes_rng rng{0};

    std::uint64_t seed = 0;

    for (int i = 0; i < 8; i++) {
        seed <<= 8;
        seed |= rng();
    }

    std::array<std::uint8_t, 159> secret;
    for (int i = 0; i < 159; i++)
        secret[i] = rng();

    auto src_file_len = std::filesystem::file_size(src_file);
    int fd = open(src_file.c_str(), O_RDONLY);
    auto bytes = (std::uint8_t*)mmap(nullptr, src_file_len, PROT_READ, MAP_PRIVATE, fd, 0);

    std::ofstream output(dst_file, std::ios::out | std::ios::binary);

    write_len_string(output, "File generated using script/hash/xxh_vectors.cpp! Do not manually edit.");

    // dump file
    write_u16_le(output, src_file_len);
    output.write(reinterpret_cast<const char *>(bytes), src_file_len);

    write_u64_le(output, seed);
    write_u16_le(output, secret.size());
    output.write(reinterpret_cast<const char *>(secret.data()), secret.size());

    write_u16_le(output, 4);

    write_u16_le(output, 2);
    output_xxh32(output, 0, bytes, src_file_len);
    output_xxh32(output, seed, bytes, src_file_len);

    write_u16_le(output, 2);
    output_xxh64(output, 0, bytes, src_file_len);
    output_xxh64(output, seed, bytes, src_file_len);

    write_u16_le(output, 3);
    output_xxh3_64(output, bytes, src_file_len);
    output_xxh3_64seed(output, seed, bytes, src_file_len);
    output_xxh3_64secret(output, secret, bytes, src_file_len);

    write_u16_le(output, 3);
    output_xxh3_128(output, bytes, src_file_len);
    output_xxh3_128seed(output, seed, bytes, src_file_len);
    output_xxh3_128secret(output, secret, bytes, src_file_len);

    munmap(bytes, src_file_len);
    close(fd);

    return 0;
}
