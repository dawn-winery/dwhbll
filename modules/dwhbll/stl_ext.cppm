module;

#include <dwhbll/stl_ext/option.h>
#include <dwhbll/stl_ext/result.h>
#include <dwhbll/stl_ext/string.h>
#include <dwhbll/stl_ext/cow.h>
#include <dwhbll/stl_ext/packs.h>
#include <dwhbll/stl_ext/ranges.h>
#include <dwhbll/stl_ext/get_set.h>
#include <dwhbll/stl_ext/utilities.h>
#include <dwhbll/stl_ext/templates.h>
#include <dwhbll/stl_ext/try.h>

export module dwhbll.stl_ext;

export namespace dwhbll::stl_ext {
    using dwhbll::stl_ext::None;
    using dwhbll::stl_ext::Some;
    using dwhbll::stl_ext::Option;

    using dwhbll::stl_ext::Ok;
    using dwhbll::stl_ext::Err;
    using dwhbll::stl_ext::Result;
    using dwhbll::stl_ext::UNIT;
    using dwhbll::stl_ext::TO_UNIT;

    using dwhbll::stl_ext::escape_non_printable;
    using dwhbll::stl_ext::replace_all;
    using dwhbll::stl_ext::split;
    using dwhbll::stl_ext::escape_string;
    using dwhbll::stl_ext::match_glob;
    using dwhbll::stl_ext::matches_patterns;

    using dwhbll::stl_ext::cow;
    using dwhbll::stl_ext::template_pack_nth;
    using dwhbll::stl_ext::erase_inplace_with_sorted_index_list;
    using dwhbll::stl_ext::erase_inplace_with_unsorted_index_list;
    using dwhbll::stl_ext::get_set;
    using dwhbll::stl_ext::store_temporary;
    using dwhbll::stl_ext::take;
    using dwhbll::stl_ext::template_info;

    namespace __detail {
        using dwhbll::stl_ext::__detail::try_traits;
    }
}
