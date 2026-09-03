#include <dwhbll/concurrency/coroutine/wrappers/file.h>
#include <dwhbll/files/executables/pe.h>

#include <dwhbll/console/logging.h>
#include <dwhbll/files/parse_utils.h>
#include <dwhbll/files/filejar/file.h>

namespace dwhbll::files::executables {
    template<typename T>
    static constexpr T align_up(T value, T alignment) {
        ASSERT(__builtin_popcountll(alignment) == 1);
        return (value + (alignment - 1)) & ~(alignment - 1);
    }

    static constexpr void write_pad_to(std::ostream &stream, const std::uint64_t size) {
        const auto pad_len = size - static_cast<uint64_t>(stream.tellp());

        constexpr char pad_buf[8] = {};

        uint64_t i = 0;

        for (; i + 8 < pad_len; i += 8)
            stream.write(pad_buf, 8);

        for (; i < pad_len; i++)
            stream.put(0);
    }

    COFF_FILE_HEADER::COFF_FILE_HEADER(std::span<uint8_t> &file) {
        machine = static_cast<MACHINE_TYPE>(read_u16_le(file));
        num_of_sections = read_u16_le(file);

        time_date_stamp = read_u32_le(file);
        pointer_to_symbol_table = read_u32_le(file);
        number_of_symbols = read_u32_le(file);

        size_of_optional_header = read_u16_le(file);
        characteristics = static_cast<IMAGE_CHARACTERISTICS>(read_u16_le(file));
    }

    void COFF_FILE_HEADER::write(std::ostream &stream) const {
        write_u16_le(stream, static_cast<uint16_t>(machine));
        write_u16_le(stream, num_of_sections);

        write_u32_le(stream, time_date_stamp);
        write_u32_le(stream, pointer_to_symbol_table);
        write_u32_le(stream, number_of_symbols);

        write_u16_le(stream, size_of_optional_header);
        write_u16_le(stream, static_cast<uint16_t>(characteristics));
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
            console::warn("data directory entry has unresolvable data! (RVA: {:#x})", virtual_address);
            console::info("Maybe an uninit region?");
            return;
        }

        data_view = image_base.subspan(addr.value(), size);
    }

    void DATA_DIRECTORY::write(std::ostream &stream) const {
        write_u32_le(stream, virtual_address);
        write_u32_le(stream, size);
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

    void OPTIONAL_HEADER::write(std::ostream &stream) {
        write_u16_le(stream, magic);

        write_u8(stream, major_linker_version);
        write_u8(stream, minor_linker_version);

        write_u32_le(stream, size_of_code);
        write_u32_le(stream, size_of_initialized_data);
        write_u32_le(stream, size_of_uninitialized_data);
        write_u32_le(stream, address_of_entry_point);
        write_u32_le(stream, base_of_code);

        if (is_pe32())
            write_u32_le(stream, base_of_data.value());

        if (is_pe32())
            write_u32_le(stream, image_base);
        else
            write_u64_le(stream, image_base);

        write_u32_le(stream, section_alignment);
        write_u32_le(stream, file_alignment);
        write_u16_le(stream, major_operating_system_version);
        write_u16_le(stream, minor_operating_system_version);
        write_u16_le(stream, major_image_version);
        write_u16_le(stream, minor_image_version);
        write_u16_le(stream, major_subsystem_version);
        write_u16_le(stream, minor_subsystem_version);
        write_u32_le(stream, win32_version_value);

        write_u32_le(stream, size_of_image);
        write_u32_le(stream, size_of_headers);
        write_u32_le(stream, check_sum);

        write_u16_le(stream, static_cast<uint16_t>(subsystem));
        write_u16_le(stream, static_cast<uint16_t>(characteristics));

        if (is_pe32()) {
            write_u32_le(stream, size_of_stack_reserve);
            write_u32_le(stream, size_of_stack_commit);
            write_u32_le(stream, size_of_heap_reserve);
            write_u32_le(stream, size_of_heap_commit);
        } else {
            write_u64_le(stream, size_of_stack_reserve);
            write_u64_le(stream, size_of_stack_commit);
            write_u64_le(stream, size_of_heap_reserve);
            write_u64_le(stream, size_of_heap_commit);
        }

        write_u32_le(stream, loader_flags);
        write_u32_le(stream, number_of_rva_and_sizes);

        for (auto& dir : directories)
            dir.write(stream);
    }

    NT_HEADER::NT_HEADER(std::span<uint8_t> &file) {
        magic = read_u32_le(file);

        if (magic != 0x4550)
            debug::panic(R"(Expected 'PE\0\0' ('\x50\x45\0\0') for NT_HEADER magic, got {:#x})", magic);

        file_header = COFF_FILE_HEADER{file};

        optional_header = OPTIONAL_HEADER{file, file_header.size_of_optional_header};
    }

    void NT_HEADER::write(std::ostream &stream) {
        write_u32_le(stream, magic);

        file_header.write(stream);

        optional_header.write(stream);
    }

    PE_SECTION::PE_SECTION(std::span<uint8_t> &file) {
        char buf[9] = {};

        for (int i = 0; i < 8; i++)
            buf[i] = static_cast<char>(read_u8(file));
        buf[8] = 0;

        name = std::string((char*)buf);

        virtual_size = read_u32_le(file);
        virtual_address = read_u32_le(file);
        size_of_raw_data = read_u32_le(file);
        pointer_to_raw_data = read_u32_le(file);
        pointer_to_relocations = read_u32_le(file);
        pointer_to_line_numbers = read_u32_le(file);
        number_of_relocations = read_u16_le(file);
        number_of_line_numbers = read_u16_le(file);
        characteristics = static_cast<SECTION_CHARACTERISTICS>(read_u32_le(file));

        if (size_of_raw_data + pointer_to_raw_data < pointer_to_raw_data)
            debug::panic("Potentially adversarial file! (size_of_raw_data + pointer_to_raw_data overflowed uint32)!");
    }

    void PE_SECTION::fill_data_view(const std::span<uint8_t> file_view) {
        raw_data_view = file_view.subspan(pointer_to_raw_data, size_of_raw_data);
    }

    void PE_SECTION::write(std::ostream &stream) {
        uint8_t buf[8] = {};

        int i = 0;
        for (const char c : name)
            buf[i++] = c;

        for (const uint8_t b : buf)
            write_u8(stream, b);

        write_u32_le(stream, virtual_size);
        write_u32_le(stream, virtual_address);
        write_u32_le(stream, size_of_raw_data);
        write_u32_le(stream, pointer_to_raw_data);
        write_u32_le(stream, pointer_to_relocations);
        write_u32_le(stream, pointer_to_line_numbers);
        write_u16_le(stream, number_of_relocations);
        write_u16_le(stream, number_of_line_numbers);
        write_u32_le(stream, static_cast<uint32_t>(characteristics));
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

            if (scn.virtual_address + scn.virtual_size <= RVA)
                continue;

            return scn;
        }

        debug::unreachable();
    }

    void PE_SECTION_TABLE::write(std::ostream &stream) {
        std::vector<PE_SECTION> secs = sections;

        std::sort(secs.begin(), secs.end(), [](const PE_SECTION &a, const PE_SECTION &b) {
            return a.pointer_to_raw_data < b.pointer_to_raw_data;
        });

        // first emit all the section datas
        for (auto& sec : sections)
            sec.write(stream);

        for (auto& sec : secs) {
            if (sec.pointer_to_raw_data == 0)
                continue; // has no raw data to emit.

            // pad to pointer
            ASSERT(static_cast<uint64_t>(stream.tellp()) <= sec.pointer_to_raw_data);
            write_pad_to(stream, sec.pointer_to_raw_data);

            stream.write(reinterpret_cast<char*>(sec.raw_data_view.data()), static_cast<std::streamsize>(sec.raw_data_view.size()));
        }
    }

    PE_SECTION & PE_IMAGE::get_section_va(uint64_t VA) {
        return sections.get_section(VA - nt_header.optional_header.image_base);
    }

    void PE_IMAGE::load_relocs() {
        // first get the location of the table
        if (!nt_header.optional_header.has_directory(DATA_DIRECTORY_TYPE::BASERELOC))
            return;

        auto& reloc_data_dir = nt_header.optional_header.get_directory(DATA_DIRECTORY_TYPE::BASERELOC);

        auto phys_addr = rva_to_phys(reloc_data_dir.virtual_address);
        auto spn = file_view.subspan(phys_addr, reloc_data_dir.size);

        while (!spn.empty()) {
            std::uint32_t page_rva;
            std::uint32_t block_size;

            page_rva = read_u32_le(spn);
            block_size = read_u32_le(spn);

            if (block_size % 2 != 0)
                debug::panic();

            auto count = (block_size - 8) / 2;

            for (std::size_t i = 0; i < count; i++) {
                auto reloc = read_u16_le(spn);

                auto offt = reloc & 0xFFF + page_rva + image_base();
                auto type = static_cast<RELOC_TYPE>((reloc >> 12) & 0xF);

                relocs.emplace_back(offt, type);
            }
        }
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

        load_relocs();
    }

    std::vector<uint8_t>& PE_IMAGE::adopt(std::vector<uint8_t> &&buffer) {
        adopted_buffers.emplace_back(std::move(buffer));

        return adopted_buffers.back();
    }

    void PE_IMAGE::write_fixup(std::ostream &stream) {
        // Dump DOS_IMAGE from original
        // TODO: Actually regenerate the DOS stub?
        stream.write(reinterpret_cast<char*>(dos_image.header_raw.data()), static_cast<std::streamsize>(dos_image.header_raw.size()));

        // assert we have the correct amount of data.
        ASSERT(stream.tellp() == dos_image.header.e_lfanew);

        // fixup the COFF headers
        nt_header.file_header.num_of_sections = sections.size();
        nt_header.file_header.size_of_optional_header = (nt_header.optional_header.is_pe32() ? 96 : 112) +
            8 * (nt_header.optional_header.directories.size());
        nt_header.optional_header.number_of_rva_and_sizes =
            nt_header.optional_header.directories.size();

        // fixup the size information
        auto file_align = static_cast<std::uint64_t>(nt_header.optional_header.file_alignment);
        auto sec_align = static_cast<std::uint64_t>(nt_header.optional_header.section_alignment);

        std::uint64_t max_sec_end_va = 0;
        // TODO: no idea how to actually compute these :xdd:
        // std::uint64_t sum_code = 0;
        // std::uint64_t sum_initialized = 0;
        // std::uint64_t sum_uninitialized = 0;
        std::uint64_t next_raw_data_ptr = align_up(static_cast<std::uint64_t>(stream.tellp())
            + 4 // magic number
            + 20 // coff header
            + nt_header.file_header.size_of_optional_header
            + 40 * nt_header.file_header.num_of_sections
            , static_cast<std::uint64_t>(nt_header.optional_header.file_alignment));

        nt_header.optional_header.size_of_headers = next_raw_data_ptr;

        for (auto& sec : sections.sections) {
            // assign raw data size as the buffer size.
            sec.size_of_raw_data = align_up(sec.raw_data_view.size(), file_align);
            sec.pointer_to_raw_data = next_raw_data_ptr;

            if (sec.pointer_to_relocations != 0)
                debug::panic("Unhandled ptr_to_relocations!");
            if (sec.pointer_to_line_numbers != 0)
                debug::panic("Unhandled ptr_to_line_numbers!");

            // TODO: maybe also align virtual_size?

            max_sec_end_va = std::max(max_sec_end_va,
                static_cast<std::uint64_t>(sec.virtual_address) + sec.virtual_size);

            // alignment unnecessary, size already aligned.
            next_raw_data_ptr = sec.pointer_to_raw_data + sec.size_of_raw_data;
        }

        nt_header.optional_header.size_of_image = align_up(max_sec_end_va, sec_align);

        // write the NT header
        nt_header.write(stream);

        // write sections
        sections.write(stream);
    }

    uint64_t PE_IMAGE::va_to_phys(uint64_t VA) const {
        return rva_to_phys(VA - image_base());
    }

    uint64_t PE_IMAGE::rva_to_phys(uint64_t RVA) const {
        for (auto& scn : sections.sections) {
            if (scn.virtual_address > RVA)
                continue;

            if (scn.virtual_address + scn.virtual_size <= RVA)
                continue;

            auto voff = RVA - scn.virtual_address;

            if (voff >= scn.size_of_raw_data)
                debug::panic("RVA: {:#x} has no corresponding physical offset (uninit/zeroed data most likely)");

            return voff + scn.pointer_to_raw_data;
        }

        debug::panic("Failed to conver RVA: {:#x} to physical offset", RVA);
    }
}
