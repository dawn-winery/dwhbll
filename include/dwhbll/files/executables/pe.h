#pragma once

#include <string>
#include <variant>
#include <vector>
#include <dwhbll/console/debug.hpp>

#include <dwhbll/files/executables/mz.h>

namespace dwhbll::files::executables {
    enum class MACHINE_TYPE : uint16_t {
        UNKNOWN = 0x0,
        ALPHA = 0x184, ///< Alpha AXP, 32-bit address space
        ALPHA64 = 0x284, ///< Alpha 64, 64-bit address space
        AXP64 = 0x284, ///< AXP 64 (Same as Alpha 64)
        AM33 = 0x1D3, ///< Matsushita AM33
        AMD64 = 0x8664, ///< x64
        ARM = 0x1C0, ///< ARM little endian
        ARM64 = 0xAA64, ///< ARM64 little endian
        ARM64EC = 0xA641, ///< ABI that enables interoperability between native ARM64 and emulated x64 code.
        ARM64X = 0xA64E, ///< Binary format that allows both native ARM64 and ARM64EC code to coexist in the same file.
        ARMNT = 0x1C4, ///< ARM Thumb-2 little endian
        EBC = 0xEBC, ///< EFI bytecode
        I386 = 0x14C, ///< I386 or compatible
        IA64 = 0x200, ///< Intel Itanium family
        LOONGARCH32 = 0x6232, ///< LoongArch 32-bit
        LOONGARCH64 = 0x6264, ///< LoongArch 64-bit
        M32R = 0x9041, ///< Mitsubishi M32R little endian
        MIPS16 = 0x266, ///< MIPS16
        MIPSFPU = 0x366,   ///< MIPS with FPU
        MIPSFPU16 = 0x466, ///< MIPS16 with FPU
        POWERPC = 0x1f0,   ///< Power PC little endian
        POWERPCFP = 0x1f1, ///< Power PC with floating point support
        POWERPCBE = 0x1f2, ///< Power PC big endian
        R3000 = 0x162,     ///< MIPS little endian, 0x160 big-endian
        R4000 = 0x166,     ///< MIPS little endian
        R10000 = 0x168,    ///< MIPS little endian
        RISCV32 = 0x5032,  ///< RISC-V 32-bit address space
        RISCV64 = 0x5064,  ///< RISC-V 64-bit address space
        RISCV128 = 0x5128, ///< RISC-V 128-bit address space
        SH3 = 0x1a2,       ///< Hitachi SH3
        SH3DSP = 0x1a3,    ///< Hitachi SH3 DSP
        SH3E = 0x1a4,      ///< Hitachi SH3E
        SH4 = 0x1a6,       ///< Hitachi SH4
        SH5 = 0x1a8,       ///< Hitachi SH5
        THUMB = 0x1c2,     ///< Thumb
        TRICORE = 0x520,   ///< Infineon
        WCEMIPSV2 = 0x169, ///< MIPS little-endian WCE v2
    };

    enum class IMAGE_CHARACTERISTICS : uint16_t {
        RELOCS_STRIPPED = 0x0001,
        EXECUTABLE_IMAGE = 0x0002,
        LINE_NUMS_STRIPPED = 0x0004,
        LOCAL_SYMS_STRIPPED = 0x0008,
        AGGRESSIVE_WS_TRIM = 0x0010,
        LARGE_ADDRESS_AWARE = 0x0020,
        BYTES_REVERSED_LO = 0x0080,
        MACHINE_32BIT = 0x0100,
        DEBUG_STRIPPED = 0x0200,
        REMOVABLE_RUN_FROM_SWAP = 0x0400,
        NET_RUN_FROM_SWAP = 0x0800,
        SYSTEM = 0x1000,
        DLL = 0x2000,
        UP_SYSTEM_ONLY = 0x4000,
        BYTES_REVERSED_HI = 0x8000,
    };

    struct COFF_FILE_HEADER {
        MACHINE_TYPE machine{};
        uint16_t num_of_sections{};
        uint32_t time_date_stamp{};
        uint32_t pointer_to_symbol_table{};
        uint32_t number_of_symbols{};
        uint16_t size_of_optional_header{};
        IMAGE_CHARACTERISTICS characteristics{};

        COFF_FILE_HEADER() = default;

        explicit COFF_FILE_HEADER(std::span<uint8_t> &file);
    };

    enum class WINDOWS_SUBSYSTEM : uint16_t {
        UNKNOWN = 0,
        NATIVE = 1,
        WINDOWS_GUI = 2,
        WINDOWS_CUI = 3,
        OS2_CUI = 5,
        POSIX_CUI = 7,
        NATIVE_WINDOWS = 8,
        WINDOWS_CE_GUI = 9,
        EFI_APPLICATION = 10,
        EFI_BOOT_SERVICE_DRIVER = 11,
        EFI_RUNTIME_DRIVER = 12,
        EFI_ROM = 13,
        XBOX = 14,
        WINDOWS_BOOT_APPLICATION = 16,
        XBOX_CODE_CATALOG = 17,
    };

    enum class DLL_CHARACTERISTICS : uint16_t {
        HIGH_ENTROPY_VA = 0x0020,
        DYNAMIC_BASE = 0x0040,
        FORCE_INTEGRITY = 0x0080,
        NX_COMPAT = 0x0100,
        NO_ISOLATION = 0x0200,
        NO_SEH = 0x0400,
        NO_BIND = 0x0800,
        APPCONTAINER = 0x1000,
        WDM_DRIVER = 0x2000,
        GUARD_CF = 0x4000,
        TERMINAL_SERVER_AWARE = 0x8000,
    };

    struct PE_SECTION_TABLE;

    struct DATA_DIRECTORY {
        constexpr static std::size_t SIZE_PER_DIR = 8;

        std::span<uint8_t> data_view{};

        uint32_t virtual_address{};
        uint32_t size{};

        DATA_DIRECTORY() = default;

        explicit DATA_DIRECTORY(std::span<uint8_t> &view);

        void fill_data_view(std::span<uint8_t> image_base, const PE_SECTION_TABLE &sections);
    };

    struct OPTIONAL_HEADER {
        uint16_t magic{};
        uint8_t major_linker_version{};
        uint8_t minor_linker_version{};
        uint32_t size_of_code{};
        uint32_t size_of_initialized_data{};
        uint32_t size_of_uninitialized_data{};
        uint32_t address_of_entry_point{};
        uint32_t base_of_code{};
        std::optional<uint32_t> base_of_data{};

        uint64_t image_base{};
        uint32_t section_alignment{};
        uint32_t file_alignment{};
        uint16_t major_operating_system_version{};
        uint16_t minor_operating_system_version{};
        uint16_t major_image_version{};
        uint16_t minor_image_version{};
        uint16_t major_subsystem_version{};
        uint16_t minor_subsystem_version{};
        uint32_t win32_version_value{};
        uint32_t size_of_image{};
        uint32_t size_of_headers{};
        uint32_t check_sum{};
        WINDOWS_SUBSYSTEM subsystem{};
        DLL_CHARACTERISTICS characteristics{};
        uint64_t size_of_stack_reserve{};
        uint64_t size_of_stack_commit{};
        uint64_t size_of_heap_reserve{};
        uint64_t size_of_heap_commit{};
        uint32_t loader_flags{};
        uint32_t number_of_rva_and_sizes{};
        std::vector<DATA_DIRECTORY> directories{};

        OPTIONAL_HEADER() = default;

        explicit OPTIONAL_HEADER(std::span<uint8_t> &file, std::uint64_t header_size_limit);

        [[nodiscard]] constexpr bool is_pe32() const {
            if (magic == 0x010B)
                return true; // PE32
            if (magic == 0x020B)
                return false; // PE32+
            debug::panic("Unknown PE magic {:#x}", magic);
        }
    };

    struct NT_HEADER {
        std::uint32_t magic{};
        COFF_FILE_HEADER file_header{};

        OPTIONAL_HEADER optional_header{};

        NT_HEADER() = default;

        explicit NT_HEADER(std::span<uint8_t> &file);
    };

    enum class SECTION_CHARACTERISTICS : uint32_t {
        TYPE_NO_PAD = 0x00000008,
        CNT_CODE = 0x00000020,
        CNT_INITIALIZED_DATA = 0x00000040,
        CNT_UNINITIALIZED_DATA = 0x00000080,
        LNK_OTHER = 0x00000100,
        LNK_INFO = 0x00000200,
        LNK_REMOVE = 0x00000800,
        LNK_COMDAT = 0x00001000,
        NO_DEFER_SPEC_EXC = 0x00004000,
        GPREL = 0x00008000,
        MEM_FARDATA = 0x00008000,
        MEM_PURGEABLE = 0x00020000,
        MEM_16BIT = 0x00020000,
        MEM_LOCKED = 0x00040000,
        MEM_PRELOAD = 0x00080000,
        ALIGN_1BYTES = 0x00100000,
        ALIGN_2BYTES = 0x00200000,
        ALIGN_4BYTES = 0x00300000,
        ALIGN_8BYTES = 0x00400000,
        ALIGN_16BYTES = 0x00500000,
        ALIGN_32BYTES = 0x00600000,
        ALIGN_64BYTES = 0x00700000,
        ALIGN_128BYTES = 0x00800000,
        ALIGN_256BYTES = 0x00900000,
        ALIGN_512BYTES = 0x00A00000,
        ALIGN_1024BYTES = 0x00B00000,
        ALIGN_2048BYTES = 0x00C00000,
        ALIGN_4096BYTES = 0x00D00000,
        ALIGN_8192BYTES = 0x00E00000,
        ALIGN_MASK = 0x00F00000,
        LNK_NRELOC_OVFL = 0x01000000,
        MEM_DISCARDABLE = 0x02000000,
        MEM_NOT_CACHED = 0x04000000,
        MEM_NOT_PAGED = 0x08000000,
        MEM_SHARED = 0x10000000,
        MEM_EXECUTE = 0x20000000,
        MEM_READ = 0x40000000,
        MEM_WRITE = 0x80000000,
    };

    struct PE_SECTION {
        std::string name{};
        uint32_t virtual_size{};
        uint32_t virtual_address{};
        uint32_t size_of_raw_data{};
        uint32_t pointer_to_raw_data{};
        uint32_t pointer_to_relocations{};
        uint32_t pointer_to_line_numbers{};
        uint16_t number_of_relocations{};
        uint16_t number_of_line_numbers{};
        SECTION_CHARACTERISTICS characteristics{};

        std::span<uint8_t> raw_data_view{}; // TODO: relocations_view, line_numbers_view;

        PE_SECTION() = default;

        explicit PE_SECTION(std::span<uint8_t> &file);

        void fill_data_view(std::span<uint8_t> file_view);
    };

    struct PE_SECTION_TABLE {
        std::vector<PE_SECTION> sections{};

        PE_SECTION_TABLE() = default;

        explicit PE_SECTION_TABLE(std::span<uint8_t> &file, std::uint16_t scn_count);

        void fill_data_views(std::span<uint8_t> file_view);

        constexpr std::vector<PE_SECTION>& get() {
            return sections;
        }

        // std::span<uint8_t> view_of_va(std::uint64_t addr);

        [[nodiscard]] std::optional<uint64_t> resolve_phys_addr(uint64_t RVA) const;

        [[nodiscard]] PE_SECTION& get_section(uint64_t RVA);
    };

    /**
     * @brief PE image
     *
     * @warning This will just panic at the slightest sign of a broken PE!!! Dont use if you want to recover errors!!
     * @warning This also won't stop you from doing dumb things with the rest of the binary!
     */
    struct PE_IMAGE {
        std::span<uint8_t> file_view;

        DOS_IMAGE dos_image{};

        NT_HEADER nt_header{};

        PE_SECTION_TABLE sections{};

        PE_IMAGE() = default;

        explicit PE_IMAGE(std::span<uint8_t> file);
    };
}
