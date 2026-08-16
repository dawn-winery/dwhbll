#include <dwhbll/files/executables/pe.h>

#include <dwhbll/console/Logging.h>
#include <dwhbll/files/parse_utils.h>

namespace dwhbll::files::executables {
    COFF_FILE_HEADER::COFF_FILE_HEADER(std::span<uint8_t> &file) {
        machine = static_cast<MACHINE_TYPE>(read_u16_le(file));
        num_of_sections = read_u16_le(file);

        time_date_stamp = read_u32_le(file);
        pointer_to_symbol_table = read_u32_le(file);
        number_of_symbols = read_u32_le(file);

        size_of_optional_header = read_u16_le(file);
        characteristics = static_cast<IMAGE_CHARACTERISTICS>(read_u16_le(file));
    }

    DATA_DIRECTORY::DATA_DIRECTORY(std::span<uint8_t> &view) {
        virtual_address = read_u32_le(view);
        size = read_u32_le(view);
    }

    void DATA_DIRECTORY::fill_data_view(const std::span<uint8_t> image_base, const PE_SECTION_TABLE &sections) {
        if (virtual_address == 0)
            return;
        const auto addr = sections.resolve_phys_addr(virtual_address);

        if (!addr.has_value()) {
            console::warn("data directory entry has unresolvable data!");
            return;
        }

        data_view = image_base.subspan(addr.value(), size);
    }

    OPTIONAL_HEADER::OPTIONAL_HEADER(std::span<uint8_t> &file, const std::uint64_t header_size_limit) {

        magic = read_u16_le(file);

        const bool pe32 = is_pe32();

        if (header_size_limit < (pe32 ? 96 : 112)) {
            debug::panic("Not enough header length to even fill the required optional header!");
        }

        const auto size_of_dirs = header_size_limit - (pe32 ? 96 : 112);

        major_linker_version = read_u8(file);
        minor_linker_version = read_u8(file);

        size_of_code = read_u32_le(file);
        size_of_initialized_data = read_u32_le(file);
        size_of_uninitialized_data = read_u32_le(file);
        address_of_entry_point = read_u32_le(file);
        base_of_code = read_u32_le(file);

        if (pe32)
            base_of_data = read_u32_le(file);

        image_base = pe32 ? read_u32_le(file) : read_u64_le(file);
        section_alignment = read_u32_le(file);
        file_alignment = read_u32_le(file);
        major_operating_system_version = read_u16_le(file);
        minor_operating_system_version = read_u16_le(file);
        major_image_version = read_u16_le(file);
        minor_image_version = read_u16_le(file);
        major_subsystem_version = read_u16_le(file);
        minor_subsystem_version = read_u16_le(file);
        win32_version_value = read_u32_le(file);

        size_of_image = read_u32_le(file);
        size_of_headers = read_u32_le(file);
        check_sum = read_u32_le(file);

        subsystem = static_cast<WINDOWS_SUBSYSTEM>(read_u16_le(file));
        characteristics = static_cast<DLL_CHARACTERISTICS>(read_u16_le(file));

        size_of_stack_reserve = pe32 ? read_u32_le(file) : read_u64_le(file);
        size_of_stack_commit = pe32 ? read_u32_le(file) : read_u64_le(file);
        size_of_heap_reserve = pe32 ? read_u32_le(file) : read_u64_le(file);
        size_of_heap_commit = pe32 ? read_u32_le(file) : read_u64_le(file);

        loader_flags = read_u32_le(file);
        number_of_rva_and_sizes = read_u32_le(file);

        const auto number_of_entries = size_of_dirs / DATA_DIRECTORY::SIZE_PER_DIR;

        if (number_of_entries > 16)
            console::warn("Too many directories! Expected maximum 16.");

        if (number_of_entries != number_of_rva_and_sizes)
            console::warn("number_of_rva_and_sizes != number_of_entries");

        const std::size_t extraneous_size = size_of_dirs % DATA_DIRECTORY::SIZE_PER_DIR;

        if (extraneous_size != 0)
            console::warn("Remaining length of optional header is not a multiple of data dir size!");

        for (int i  = 0; i < size_of_dirs; i += DATA_DIRECTORY::SIZE_PER_DIR)
            directories.emplace_back(file);

        // consume extraneous size
        // Even if the size is too big per file header, we should consume the extras just in case.
        // TODO: does windows loader do this?
        if (extraneous_size != 0)
            file = file.subspan(extraneous_size);
    }

    NT_HEADER::NT_HEADER(std::span<uint8_t> &file) {
        magic = read_u32_le(file);

        if (magic != 0x4550)
            debug::panic(R"(Expected 'PE\0\0' ('\x50\x45\0\0') for NT_HEADER magic, got {:#x})", magic);

        file_header = COFF_FILE_HEADER{file};

        optional_header = OPTIONAL_HEADER{file, file_header.size_of_optional_header};
    }

    PE_SECTION::PE_SECTION(std::span<uint8_t> &file) {
        std::string str;

        for (int i = 0; i < 8; i++) {
            const auto c = read_u8(file);

            if (c != 0)
                str += static_cast<char>(c);
            else
                break;
        }

        name = str;

        virtual_size = read_u32_le(file);
        virtual_address = read_u32_le(file);
        size_of_raw_data = read_u32_le(file);
        pointer_to_raw_data = read_u32_le(file);
        pointer_to_relocations = read_u32_le(file);
        pointer_to_line_numbers = read_u32_le(file);
        number_of_relocations = read_u32_le(file);
        number_of_line_numbers = read_u32_le(file);
        characteristics = static_cast<SECTION_CHARACTERISTICS>(read_u32_le(file));

        if (size_of_raw_data + pointer_to_raw_data < pointer_to_raw_data)
            debug::panic("Potentially adversarial file! (size_of_raw_data + pointer_to_raw_data overflowed uint32)!");
    }

    void PE_SECTION::fill_data_view(const std::span<uint8_t> file_view) {
        raw_data_view = file_view.subspan(pointer_to_raw_data, size_of_raw_data);
    }

    PE_SECTION_TABLE::PE_SECTION_TABLE(std::span<uint8_t> &file, const std::uint16_t scn_count) {
        for (int i = 0; i < scn_count; i++)
            sections.emplace_back(file);
    }

    void PE_SECTION_TABLE::fill_data_views(const std::span<uint8_t> file_view) {
        for (auto& scn : sections)
            scn.fill_data_view(file_view);
    }

    std::optional<uint64_t> PE_SECTION_TABLE::resolve_phys_addr(const std::uint64_t RVA) const {
        for (auto& scn : sections) {
            if (scn.virtual_address > RVA)
                continue;

            if (scn.virtual_address + scn.size_of_raw_data <= RVA)
                continue;

            const auto from_scn_base = RVA - scn.virtual_address;

            return from_scn_base + scn.pointer_to_raw_data;
        }

        return std::nullopt;
    }

    PE_SECTION & PE_SECTION_TABLE::get_section(const uint64_t RVA) {
        for (auto& scn : sections) {
            if (scn.virtual_address > RVA)
                continue;

            if (scn.virtual_address + scn.size_of_raw_data <= RVA)
                continue;

            return scn;
        }

        debug::unreachable();
    }

    PE_IMAGE::PE_IMAGE(const std::span<uint8_t> file) {
        file_view = file;

        dos_image = DOS_IMAGE(file);

        auto parse_view = file.subspan(dos_image.header.e_lfanew);
        nt_header = NT_HEADER{parse_view};
        sections = PE_SECTION_TABLE{parse_view, nt_header.file_header.num_of_sections};

        // fill section views
        sections.fill_data_views(file_view);

        for (auto& dir : nt_header.optional_header.directories)
            dir.fill_data_view(file_view, sections);
    }
}
