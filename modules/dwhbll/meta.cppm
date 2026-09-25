module;

#include <dwhbll/meta/meta.h>

export module dwhbll.meta;

export namespace dwhbll::meta {
    using dwhbll::meta::fixed_string;
    using dwhbll::meta::enum_to_string;
    using dwhbll::meta::string_to_enum;
    using dwhbll::meta::find_annotation;
    using dwhbll::meta::has_annotation;
    using dwhbll::meta::is_reserved_name;
    using dwhbll::meta::is_std_or_reserved;
    using dwhbll::meta::collect_annotated;
}
