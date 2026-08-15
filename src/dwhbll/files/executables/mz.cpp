#include <dwhbll/files/executables/mz.h>

#include <dwhbll/files/parse_utils.h>

namespace dwhbll::files::executables {
    DOS_HEADER::DOS_HEADER() = default;

    DOS_HEADER::DOS_HEADER(std::span<uint8_t> file) {
        // TODO: technically the MZ header can be smaller for non PE files
        // but that's a problem for future me.
        e_magic = read_u16_le(file);
        e_cblp = read_u16_le(file);
        e_cp = read_u16_le(file);
        e_crlc = read_u16_le(file);
        e_cparhdr = read_u16_le(file);
        e_minalloc = read_u16_le(file);
        e_maxalloc = read_u16_le(file);
        e_ss = read_u16_le(file);
        e_sp = read_u16_le(file);
        e_csum = read_u16_le(file);
        e_ip = read_u16_le(file);
        e_cs = read_u16_le(file);
        e_lfarlc = read_u16_le(file);
        e_ovno = read_u16_le(file);

        for (unsigned short &i : e_res) {
            i = read_u16_le(file);
        }

        e_oemid = read_u16_le(file);
        e_oeminfo = read_u16_le(file);

        for (unsigned short &i : e_res2)
            i = read_u16_le(file);

        e_lfanew = read_u32_le(file);
    }

    DOS_IMAGE::DOS_IMAGE() = default;

    DOS_IMAGE::DOS_IMAGE(std::span<uint8_t> file) : header(file) {
        // compute the range for the DOS stub
        auto begin = header.e_cparhdr * 16;
        auto end = file.size();

        if (header.e_lfanew != 0)
            end = header.e_lfanew;

        end -= begin;

        data = file.subspan(begin, end);
    }
}
