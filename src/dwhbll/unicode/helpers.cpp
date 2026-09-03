#include <ranges>
#include <dwhbll/unicode/helpers.h>

#include <dwhbll/unicode/table.h>

// TODO: This entire file has a lot of redundancies, probably worth cleaning up.
namespace dwhbll::unicode {
    namespace normalization {
        [[nodiscard]] bool is_hangul_syllable(char32_t c) {
            return c >= hangul::SBASE && c < (hangul::SBASE + hangul::SCOUNT);
        }

        std::pair<std::array<char32_t, 3>, size_t> decompose_hangul(char32_t c) {
            const char32_t s_index = c - hangul::SBASE;
            const char32_t l = hangul::LBASE + (s_index / hangul::NCOUNT);
            const char32_t v = hangul::VBASE + ((s_index % hangul::NCOUNT) / hangul::TCOUNT);
            const char32_t t = s_index % hangul::TCOUNT;

            std::array<char32_t, 3> result{l, v};
            if (t > 0)
                result[2] = hangul::TBASE + t;

            return {result, t > 0 ? 3 : 2};
        }

        char32_t compose_hangul(char32_t first, char32_t second) {
            // L + V -> LV Syllable
            if (first >= hangul::LBASE && first < (hangul::LBASE + hangul::LCOUNT) &&
                second >= hangul::VBASE && second < (hangul::VBASE + hangul::VCOUNT)) {
                const char32_t l_idx = first - hangul::LBASE;
                const char32_t v_idx = second - hangul::VBASE;
                return hangul::SBASE + (l_idx * hangul::VCOUNT + v_idx) * hangul::TCOUNT;
            }

            // LV + T -> LVT Syllable
            if (is_hangul_syllable(first) && ((first - hangul::SBASE) %hangul::TCOUNT == 0) &&
                second > hangul::TBASE && second < (hangul::TBASE + hangul::TCOUNT)) {
                const char32_t t_idx = second - hangul::TBASE;
                return first + t_idx;
            }

            return 0; // not valid
        }

        std::vector<char32_t> decompose(char32_t c, bool accept_compat) {
            if (is_hangul_syllable(c)) {
                auto [decomp, size] =  decompose_hangul(c);

                return {decomp.begin(), decomp.begin() + size};
            }

            if (accept_compat) {
                auto pos = base::compat_decomposition_table.find(c);
                if (pos != base::compat_decomposition_table.end())
                    return pos->data;
            } else {
                auto pos = base::decomposition_table.find(c);
                if (pos != base::decomposition_table.end())
                    return pos->data;
            }

            return {};
        }

        void canonical_ordering_subblock(std::span<char32_t> spn) {
            if (spn.size() <= 1)
                debug::unreachable();

            struct KeyPair {
                char32_t key;
                int value;
            };

            std::vector<KeyPair> keys(spn.size());
            for (auto [index, c] : spn | std::views::enumerate) {
                if (auto val = base::canonical_combining_class.find(c);
                    val != base::canonical_combining_class.end())
                    keys[index] = {c, val->data};
                else
                    debug::unreachable();
            }

            std::ranges::stable_sort(keys,
                [](const auto& left, const auto& right) {
                    return left.value < right.value;
                });

            for (size_t i = 0; i < spn.size(); i++)
                spn[i] = keys[i].key;
        }

        void canonical_ordering(std::span<char32_t> spn) {
            size_t last_ccc0 = 0;
            size_t i;
            int curccc;
            bool lacking = !spn.empty() && (base::canonical_combining_class.at_or_default(spn[0], 0));

            for (i = 0; i < spn.size(); i++) {
                curccc = base::canonical_combining_class.at_or_default(spn[i], 0);

                if (curccc == 0 && last_ccc0 == 0 && i > 1 && lacking) {
                    // is is possible that there was no leading CCC 0 at all
                    canonical_ordering_subblock(spn.subspan(0, i));
                } else if (curccc == 0 && i - last_ccc0 > 2) {
                    // last starter is more than 2 away, this means {last_ccc0, first, second, i}
                    // Otherwise there's nothing worth sorting
                    canonical_ordering_subblock(spn.subspan(last_ccc0 + 1, i - last_ccc0 - 1));
                }

                if (curccc == 0)
                    last_ccc0 = i;
            }

            // handle last in span
            if (curccc != 0 && i - last_ccc0 > 2)
                canonical_ordering_subblock(spn.subspan(last_ccc0 + 1, i - last_ccc0 - 1));
        }

        void canonical_composition(std::span<char32_t> &str) {
            if (str.empty()) return;

            size_t starter_idx = 0;
            uint8_t last_ccc = base::canonical_combining_class.at_or_default(str[0], 0);
            size_t write_idx = 1;

            for (size_t read_idx = 1; read_idx < str.size(); read_idx++) {
                char32_t ch = str[read_idx];
                uint8_t ccc = base::canonical_combining_class.at_or_default(ch, 0);
                char32_t starter = str[starter_idx];

                // Blocked check condition:
                // Unblocked if immediately adjacent or if preceding character
                // CCC < current character CCC
                bool unblocked = (starter_idx == write_idx - 1) || (last_ccc < ccc);

                char32_t composite = 0;

                if (unblocked) {
                    composite = compose_hangul(starter, ch);

                    if (composite == 0) {
                        // look up the starter in our database
                        auto secondary_table_it = base::composition_table.find(starter);

                        if (secondary_table_it != base::composition_table.end()) {
                            // starter found
                            auto [start, end] = secondary_table_it->data;

                            for (auto it = start; it != end; it++) {
                                auto [second, composed] = *it;

                                if (second == ch) {
                                    // matching composition
                                    composite = composed;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (composite != 0) {
                    // replace starter with new composite
                    str[starter_idx] = composite;
                } else {
                    if (ccc == 0)
                        starter_idx = write_idx; // new starter

                    last_ccc = ccc;
                    str[write_idx++] = ch;
                }
            }

            str = str.subspan(0, write_idx);
        }

        namespace nfc {
            std::u32string normalize(std::u32string_view str) {
                size_t i = 0;
                size_t n = str.size();

                // O(n) copy as long as the string passes QC
                uint8_t prev_ccc = 0;
                while (i < n) {
                    auto qc = nfc_qc.at_or_default(str[i], QC_VAL::YES);

                    // QC_YES is not contained in table
                    // Not QC_YES means NO or MAYBE
                    if (qc != QC_VAL::YES)
                        break;

                    auto cccv = base::canonical_combining_class.find(str[i]);
                    auto ccc = cccv == base::canonical_combining_class.end() ? 0 : cccv->data;

                    if (prev_ccc > ccc && ccc != 0)
                        break;

                    prev_ccc = ccc;
                    i++;
                }

                if (i == n)
                    return std::u32string(str);

                // Encountered QC_MAYBE or QC_NO
                // Should still be fairly close to O(n) on average but
                // approaches O(8n) if all decompositions are rare, large decompositions

                size_t last_starter = i;
                while (last_starter > 0) {
                    auto cccv = base::canonical_combining_class.find(str[last_starter]);
                    auto ccc = cccv == base::canonical_combining_class.end() ? 0 : cccv->data;
                    auto qcv = nfc_qc.find(str[last_starter]);
                    auto qc =  qcv == nfc_qc.end() ? QC_VAL::YES : qcv->data;

                    if (ccc == 0 && qc == QC_VAL::YES)
                        break;
                    last_starter--;
                }

                std::u32string result(str, 0, last_starter);

                // decompose remaining chars
                for (size_t k = last_starter; k < n; k++) {
                    auto r = decompose(str[k], false);

                    if (r.empty())
                        result.push_back(str[k]);
                    else
                        result.append_range(r);
                }

                std::span spn(result.begin() + last_starter, result.end());

                canonical_ordering(spn);
                canonical_composition(spn);

                // composition modified our span to point to the new resulting data.
                // this means we should resize our result to match that expectation
                result.resize(last_starter + spn.size());

                return result;
            }

            bool validate_nfc_maybe(std::span<const char32_t> str) {
                std::u32string result;
                result.reserve(str.size() * 2);

                for (const char32_t k : str) {
                    auto r = decompose(k, false);

                    if (r.empty())
                        result.push_back(k);
                    else
                        result.append_range(r);
                }

                std::span spn(result.begin(), result.end());

                canonical_ordering(spn);
                canonical_composition(spn);

                result.resize(spn.size());

                return std::ranges::equal(str, result);
            }

            bool quick_check(std::u32string_view str) {
                uint8_t prev_ccc = 0;
                size_t n = str.size();
                size_t last_starter_idx = 0;

                for (size_t i = 0; i < n; ++i) {
                    char32_t c = str[i];

                    if (c < 0x80) {
                        prev_ccc = 0;
                        last_starter_idx = i;
                        continue;
                    }

                    auto ccc = base::canonical_combining_class.at_or_default(c, 0);

                    if (ccc == 0)
                        last_starter_idx = i;

                    if (prev_ccc > ccc && ccc != 0)
                        return false;

                    prev_ccc = ccc;

                    auto qc = nfc_qc.at_or_default(c, QC_VAL::YES);

                    if (qc == QC_VAL::NO)
                        return false;

                    if (qc == QC_VAL::MAYBE) {
                        size_t end = i + 1;
                        while (end < n && base::canonical_combining_class.at_or_default(str[end], 0) != 0)
                            end++;

                        if (!validate_nfc_maybe(std::span{
                            str.begin() + last_starter_idx,
                            end - last_starter_idx
                        }))
                            return false;

                        i = end - 1;
                        prev_ccc = base::canonical_combining_class.at_or_default(c, 0);
                    }
                }

                return true;
            }
        }

        namespace nfd {
            std::u32string normalize(std::u32string_view str) {
                size_t i = 0;
                size_t n = str.size();

                // 1. Fast Path: Find first code point violating NFD QC or ordering
                uint8_t prev_ccc = 0;
                while (i < n) {
                    auto ch = str[i];
                    auto qcv = nfd_qc.at_or_default(ch, QC_VAL::YES);

                    if (qcv == QC_VAL::NO)
                        break;

                    uint8_t ccc = base::canonical_combining_class.at_or_default(ch, 0);
                    if (prev_ccc > ccc && ccc != 0)
                        break; // fail canonical ordering

                    prev_ccc = ccc;
                    i++;
                }

                if (i == n)
                    return std::u32string(str);

                // Similar to NFC normalization this path should be close to O(n)
                // however, in a similar issue, if all chars are problematic then
                // decomposition can take longer, closer to O(5n)
                // Backtrack to starter
                while (i > 0 && base::canonical_combining_class.at_or_default(str[i], 0) != 0)
                    i--;

                std::u32string result(str, 0, i);

                // decompose remaining chars
                for (size_t k = i; k < n; k++) {
                    auto r = decompose(str[k], false);

                    if (r.empty())
                        result.push_back(str[k]);
                    else
                        result.append_range(r);
                }

                std::span spn(result.begin() + i, result.end());

                canonical_ordering(spn);

                return result;
            }

            bool quick_check(std::u32string_view str) {
                uint8_t prev_ccc = 0;

                for (char32_t c : str) {
                    // ASCII
                    if (c < 0x80) {
                        prev_ccc = 0;
                        continue;
                    }

                    auto qc = nfd_qc.at_or_default(c, QC_VAL::YES);

                    if (qc == QC_VAL::NO)
                        return false;

                    auto ccc = base::canonical_combining_class.at_or_default(c, 0);

                    if (prev_ccc > ccc && ccc != 0)
                        return false;

                    prev_ccc = ccc;
                }

                return true;
            }
        }

        namespace nfkc {
            std::u32string normalize(std::u32string_view str) {
                size_t i = 0;
                size_t n = str.size();

                // O(n) copy as long as the string passes QC
                uint8_t prev_ccc = 0;
                while (i < n) {
                    auto qc = nfkc_qc.at_or_default(str[i], QC_VAL::YES);

                    // QC_YES is not contained in table
                    // Not QC_YES means NO or MAYBE
                    if (qc != QC_VAL::YES)
                        break;

                    auto cccv = base::canonical_combining_class.find(str[i]);
                    auto ccc = cccv == base::canonical_combining_class.end() ? 0 : cccv->data;

                    if (prev_ccc > ccc && ccc != 0)
                        break;

                    prev_ccc = ccc;
                    i++;
                }

                if (i == n)
                    return std::u32string(str);

                // Encountered QC_MAYBE or QC_NO
                // Should still be fairly close to O(n) on average but
                // approaches O(8n) if all decompositions are rare, large decompositions

                size_t last_starter = i;
                while (last_starter > 0) {
                    auto cccv = base::canonical_combining_class.find(str[last_starter]);
                    auto ccc = cccv == base::canonical_combining_class.end() ? 0 : cccv->data;
                    auto qcv = nfkc_qc.find(str[last_starter]);
                    auto qc =  qcv == nfkc_qc.end() ? QC_VAL::YES : qcv->data;

                    if (ccc == 0 && qc == QC_VAL::YES)
                        break;
                    last_starter--;
                }

                std::u32string result(str, 0, last_starter);

                // decompose remaining chars
                for (size_t k = last_starter; k < n; k++) {
                    auto r = decompose(str[k], true);

                    if (r.empty())
                        result.push_back(str[k]);
                    else
                        result.append_range(r);
                }

                std::span spn(result.begin() + last_starter, result.end());

                canonical_ordering(spn);
                canonical_composition(spn);

                // composition modified our span to point to the new resulting data.
                // this means we should resize our result to match that expectation
                result.resize(last_starter + spn.size());

                return result;
            }

            bool validate_nfkc_maybe(std::span<const char32_t> str) {
                std::u32string result;
                result.reserve(str.size() * 2);

                for (const char32_t k : str) {
                    if (auto r = decompose(k, true); r.empty())
                        result.push_back(k);
                    else
                        result.append_range(r);
                }

                std::span spn(result.begin(), result.end());

                canonical_ordering(spn);
                canonical_composition(spn);

                result.resize(spn.size());

                return std::ranges::equal(str, result);
            }

            bool quick_check(std::u32string_view str) {
                uint8_t prev_ccc = 0;
                size_t n = str.size();
                size_t last_starter_idx = 0;

                for (size_t i = 0; i < n; ++i) {
                    char32_t c = str[i];

                    if (c < 0x80) {
                        prev_ccc = 0;
                        last_starter_idx = i;
                        continue;
                    }

                    auto ccc = base::canonical_combining_class.at_or_default(c, 0);

                    if (ccc == 0)
                        last_starter_idx = i;

                    if (prev_ccc > ccc && ccc != 0)
                        return false;

                    prev_ccc = ccc;

                    auto qc = nfkc_qc.at_or_default(c, QC_VAL::YES);

                    if (qc == QC_VAL::NO)
                        return false;

                    if (qc == QC_VAL::MAYBE) {
                        size_t end = i + 1;
                        while (end < n && base::canonical_combining_class.at_or_default(str[end], 0) != 0)
                            end++;

                        if (!validate_nfkc_maybe(std::span{
                            str.begin() + last_starter_idx,
                            end - last_starter_idx
                        }))
                            return false;

                        i = end - 1;
                        prev_ccc = base::canonical_combining_class.at_or_default(c, 0);
                    }
                }

                return true;
            }
        }

        namespace nfkd {
            std::u32string normalize(std::u32string_view str) {
                size_t i = 0;
                size_t n = str.size();

                // 1. Fast Path: Find first code point violating NFD QC or ordering
                uint8_t prev_ccc = 0;
                while (i < n) {
                    auto ch = str[i];
                    auto qcv = nfkd_qc.at_or_default(ch, QC_VAL::YES);

                    if (qcv == QC_VAL::NO)
                        break;

                    uint8_t ccc = base::canonical_combining_class.at_or_default(ch, 0);
                    if (prev_ccc > ccc && ccc != 0)
                        break; // fail canonical ordering

                    prev_ccc = ccc;
                    i++;
                }

                if (i == n)
                    return std::u32string(str);

                // Similar to NFC normalization this path should be close to O(n)
                // however, in a similar issue, if all chars are problematic then
                // decomposition can take longer, closer to O(5n)
                // Backtrack to starter
                while (i > 0 && base::canonical_combining_class.at_or_default(str[i], 0) != 0)
                    i--;

                std::u32string result(str, 0, i);

                // decompose remaining chars
                for (size_t k = i; k < n; k++) {
                    auto r = decompose(str[k], true);

                    if (r.empty())
                        result.push_back(str[k]);
                    else
                        result.append_range(r);
                }

                std::span spn(result.begin() + i, result.end());

                canonical_ordering(spn);

                return result;
            }

            bool quick_check(std::u32string_view str) {
                uint8_t prev_ccc = 0;

                for (char32_t c : str) {
                    // ASCII
                    if (c < 0x80) {
                        prev_ccc = 0;
                        continue;
                    }

                    auto qc = nfkd_qc.at_or_default(c, QC_VAL::YES);

                    if (qc == QC_VAL::NO)
                        return false;

                    auto ccc = base::canonical_combining_class.at_or_default(c, 0);

                    if (prev_ccc > ccc && ccc != 0)
                        return false;

                    prev_ccc = ccc;
                }

                return true;
            }
        }
    }
}