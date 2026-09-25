module;

#include <dwhbll/files/binary_file.h>
#include <dwhbll/files/parse_utils.h>
#include <dwhbll/files/executables/mz.h>
#include <dwhbll/files/executables/pe.h>
#include <dwhbll/files/filejar/file.h>
#include <dwhbll/files/filejar/file_mgr.h>

export module dwhbll.files;

export namespace dwhbll::files {
    using dwhbll::files::binary_file;
    using dwhbll::files::EOLType;
    using dwhbll::files::ParseUtils;
    using dwhbll::files::read_u8;
    using dwhbll::files::read_u16_le;
    using dwhbll::files::read_u32_le;
    using dwhbll::files::read_u64_le;
    using dwhbll::files::write_u8;
    using dwhbll::files::write_u16_le;
    using dwhbll::files::write_u32_le;
    using dwhbll::files::write_u64_le;

    namespace executables {
        // MZ
        using dwhbll::files::executables::DOS_HEADER;
        using dwhbll::files::executables::DOS_IMAGE;

        // PE
        using dwhbll::files::executables::MACHINE_TYPE;
        using dwhbll::files::executables::IMAGE_CHARACTERISTICS;
        using dwhbll::files::executables::COFF_FILE_HEADER;
        using dwhbll::files::executables::WINDOWS_SUBSYSTEM;
        using dwhbll::files::executables::DLL_CHARACTERISTICS;
        using dwhbll::files::executables::DATA_DIRECTORY;
        using dwhbll::files::executables::DATA_DIRECTORY_TYPE;
        using dwhbll::files::executables::OPTIONAL_HEADER;
        using dwhbll::files::executables::NT_HEADER;
        using dwhbll::files::executables::SECTION_CHARACTERISTICS;
        using dwhbll::files::executables::PE_SECTION;
        using dwhbll::files::executables::PE_SECTION_TABLE;
        using dwhbll::files::executables::RELOC_TYPE;
        using dwhbll::files::executables::PE_IMAGE;
    }

    namespace filejar {
        using dwhbll::files::filejar::file;
        using dwhbll::files::filejar::fileid;
        using dwhbll::files::filejar::file_mgr;
    }
}
