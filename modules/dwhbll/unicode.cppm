module;

#include <dwhbll/unicode/helpers.h>
#include <dwhbll/unicode/table.h>

export module dwhbll.unicode;

export namespace dwhbll::unicode {
    namespace normalization {
        using dwhbll::unicode::normalization::is_hangul_syllable;
        using dwhbll::unicode::normalization::decompose_hangul;
        using dwhbll::unicode::normalization::compose_hangul;
        using dwhbll::unicode::normalization::decompose;
        using dwhbll::unicode::normalization::canonical_ordering;
        using dwhbll::unicode::normalization::canonical_composition;

        namespace nfc {
            using dwhbll::unicode::normalization::nfc::normalize;
            using dwhbll::unicode::normalization::nfc::quick_check;
        }

        namespace nfd {
            using dwhbll::unicode::normalization::nfd::normalize;
            using dwhbll::unicode::normalization::nfd::quick_check;
        }

        namespace nfkc {
            using dwhbll::unicode::normalization::nfkc::normalize;
            using dwhbll::unicode::normalization::nfkc::quick_check;
        }

        namespace nfkd {
            using dwhbll::unicode::normalization::nfkd::normalize;
            using dwhbll::unicode::normalization::nfkd::quick_check;
        }
    }

    using dwhbll::unicode::table;
    using dwhbll::unicode::empty_struct;

    namespace properties {
        using dwhbll::unicode::properties::XID_Start;
        using dwhbll::unicode::properties::XID_Continue;
        using dwhbll::unicode::properties::ID_Compat_Math_Start;
        using dwhbll::unicode::properties::ID_Compat_Math_Continue;
    }

    namespace aliases {
        using dwhbll::unicode::aliases::name_aliases_to_codepoint;
    }

    namespace base {
        using dwhbll::unicode::base::canonical_combining_class;
        using dwhbll::unicode::base::decomposition;
        using dwhbll::unicode::base::composition;
        using dwhbll::unicode::base::compositions;
        using dwhbll::unicode::base::decomposition_table;
        using dwhbll::unicode::base::compat_decomposition_table;
        using dwhbll::unicode::base::composition_table;
    }

    namespace normalization {
        using dwhbll::unicode::normalization::QC_VAL;
        using dwhbll::unicode::normalization::nfc_qc;
        using dwhbll::unicode::normalization::nfkc_qc;
        using dwhbll::unicode::normalization::nfd_qc;
        using dwhbll::unicode::normalization::nfkd_qc;
    }

    namespace hangul {
        using dwhbll::unicode::hangul::LBASE;
        using dwhbll::unicode::hangul::SBASE;
        using dwhbll::unicode::hangul::VBASE;
        using dwhbll::unicode::hangul::TBASE;
        using dwhbll::unicode::hangul::LCOUNT;
        using dwhbll::unicode::hangul::TCOUNT;
        using dwhbll::unicode::hangul::VCOUNT;
        using dwhbll::unicode::hangul::NCOUNT;
        using dwhbll::unicode::hangul::SCOUNT;
    }
}
