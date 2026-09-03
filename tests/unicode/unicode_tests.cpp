#include <fstream>
#include <functional>
#include <iostream>
#include <unordered_set>
#include <vector>

#include <dwhbll/console/debug.hpp>
#include <dwhbll/console/Logging.h>
#include <dwhbll/unicode/helpers.h>


std::string trim(const std::string &str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start))
        ++start;
    auto end = str.end();
    do {
        --end;
    } while (end != start && std::isspace(*end));
    return std::string(start, end + 1);
}

std::string format_codepoints(const std::u32string &str) {
    std::string data;

    for (char32_t c : str)
        data += std::format("U+{:04X} ", (uint32_t)c);

    return data;
}

std::pair<bool, std::string> test_norm(
    const std::u32string &c1,
    const std::u32string &c2,
    const std::u32string &c3,
    const std::u32string &c4,
    const std::u32string &c5
) {
    bool pass = true;
    std::string data;

    // NFC conformance
    auto test_nfc = [&](const std::u32string &base, const std::u32string &rhs, const std::string &descr) {
        auto norm = dwhbll::unicode::normalization::nfc::normalize(rhs);

        if (!std::ranges::equal(base, norm)) {
            pass = false;
            data += std::format("FAIL NFC: {}, {} != {} (from {})\n", descr, format_codepoints(base), format_codepoints(norm), format_codepoints(rhs));
        }
    };
    test_nfc(c2, c1, "c2 != NFC(c1)");
    test_nfc(c2, c2, "c2 != NFC(c2)");
    test_nfc(c2, c3, "c2 != NFC(c3)");
    test_nfc(c4, c4, "c4 != NFC(c4)");
    test_nfc(c4, c5, "c4 != NFC(c5)");

    // NFD conformance
    auto test_nfd = [&](const std::u32string &base, const std::u32string &rhs, const std::string &descr) {
        auto norm = dwhbll::unicode::normalization::nfd::normalize(rhs);

        if (!std::ranges::equal(base, norm)) {
            pass = false;
            data += std::format("FAIL NFD: {}, {} != {} (from {})\n", descr, format_codepoints(base), format_codepoints(norm), format_codepoints(rhs));
        }
    };
    test_nfd(c3, c1, "c3 != NFC(c1)");
    test_nfd(c3, c2, "c3 != NFC(c2)");
    test_nfd(c3, c3, "c3 != NFC(c3)");
    test_nfd(c5, c4, "c5 != NFC(c4)");
    test_nfd(c5, c5, "c5 != NFC(c5)");

    // NFKC conformance
    auto test_nfkc = [&](const std::u32string &base, const std::u32string &rhs, const std::string &descr) {
        auto norm = dwhbll::unicode::normalization::nfkc::normalize(rhs);

        if (!std::ranges::equal(base, norm)) {
            pass = false;
            data += std::format("FAIL NFKC: {}, {} != {} (from {})\n", descr, format_codepoints(base), format_codepoints(norm), format_codepoints(rhs));
        }
    };
    test_nfkc(c4, c1, "c4 != NFC(c1)");
    test_nfkc(c4, c2, "c4 != NFC(c2)");
    test_nfkc(c4, c3, "c4 != NFC(c3)");
    test_nfkc(c4, c4, "c4 != NFC(c4)");
    test_nfkc(c4, c5, "c4 != NFC(c5)");

    // NFKD conformance
    auto test_nfkd = [&](const std::u32string &base, const std::u32string &rhs, const std::string &descr) {
        auto norm = dwhbll::unicode::normalization::nfkd::normalize(rhs);

        if (!std::ranges::equal(base, norm)) {
            pass = false;
            data += std::format("FAIL NFKD: {}, {} != {} (from {})\n", descr, format_codepoints(base), format_codepoints(norm), format_codepoints(rhs));
        }
    };
    test_nfkd(c5, c1, "c5 != NFC(c1)");
    test_nfkd(c5, c2, "c5 != NFC(c2)");
    test_nfkd(c5, c3, "c5 != NFC(c3)");
    test_nfkd(c5, c4, "c5 != NFC(c4)");
    test_nfkd(c5, c5, "c5 != NFC(c5)");

    // test QC
    auto test_qc = [&](const std::u32string &base, std::function<bool(std::u32string_view)> func, const std::string &descr) {
        if (!func(base)) {
            pass = false;
            data += std::format("FAIL QC FAIL: {}, {}\n", descr, format_codepoints(base));
        }
    };

    test_qc(c2, dwhbll::unicode::normalization::nfc::quick_check, "NFC(c2)");
    test_qc(c4, dwhbll::unicode::normalization::nfc::quick_check, "NFC(c4)");
    test_qc(c3, dwhbll::unicode::normalization::nfd::quick_check, "NFD(c3)");
    test_qc(c5, dwhbll::unicode::normalization::nfd::quick_check, "NFD(c5)");
    test_qc(c4, dwhbll::unicode::normalization::nfkc::quick_check, "NFKC(c4)");
    test_qc(c5, dwhbll::unicode::normalization::nfkd::quick_check, "NFKD(c5)");

    return {pass, data};
}

bool unicode_norm_test(std::optional<std::string> norm_file) {
    // std::u32string str = U"\u0344";
    // if (dwhbll::unicode::normalization::nfc::normalize(str) != U"\u0308\u0301")
    //     dwhbll::debug::unreachable();
    //
    // return true;

    if (!norm_file.has_value()) {
        dwhbll::console::fatal("Need the NormalizationTest.txt file!");

        return false;
    }

    std::ifstream file(norm_file.value());

    if (!file.is_open()) {
        dwhbll::console::fatal("Unable to open file {}!", norm_file.value());

        return false;
    }

    bool part1 = false;
    std::unordered_set<char32_t> individual_tested;

    size_t pass = 0;
    size_t total = 0;

    size_t overall_pass = 0;
    size_t overall = 0;

    std::string line;
    while (std::getline(file, line)) {
        // trim away any comments
        auto comment_start = line.find_first_of('#');

        if (comment_start != std::string::npos)
            line = line.substr(0, comment_start);

        line = trim(line);

        if (line.starts_with('@')) {
            if (total != 0) {
                std::cout << std::endl;
                dwhbll::console::info("PASS: {}/{}, FAIL: {}/{}", pass, total, total - pass, total);

                overall_pass += pass;
                overall += total;
                pass = 0;
                total = 0;
            }

            dwhbll::console::info("Now testing {}", line);

            part1 = false;

            if (line == "@Part1")
                part1 = true;

            continue;
        }

        if (line.empty())
            continue;

        auto cleaned_line = line;

        std::vector<std::string> data;

        for (int i = 0; i < 5; i++) {
            auto semicolon = line.find_first_of(';');

            if (semicolon == std::string::npos)
                dwhbll::debug::panic("Malformed NormalizationTest.txt line \"{}\", missing semicolons, expected 5", cleaned_line);

            data.push_back(trim(line.substr(0, semicolon)));

            line = line.substr(semicolon + 1);
        }

        if (data.size() != 5)
            dwhbll::debug::panic("Malformed NormalizationTest.txt line \"{}\", expected semicolon at end of line.", cleaned_line);

        std::vector<std::u32string> parsed_points;

        // parse codepoints
        for (int i = 0; i < 5; i++) {
            std::u32string block;
            auto str = data[i];
            while (true) {
                auto space = str.find_first_of(' ');

                // parse one
                block.push_back((char32_t)std::stoul(str.substr(0, space), nullptr, 16));

                if (space == std::string::npos)
                    break; // done
                str = str.substr(space + 1);
            }

            parsed_points.push_back(block);
        }

        auto c1 = parsed_points[0];
        auto c2 = parsed_points[1];
        auto c3 = parsed_points[2];
        auto c4 = parsed_points[3];
        auto c5 = parsed_points[4];

        if (c1.empty() || c2.empty() || c3.empty() || c4.empty() || c5.empty())
            dwhbll::debug::panic("Malformed NormalizationTest.txt line \"{}\", empty block.", cleaned_line);

        if (part1 && c1.size() != 1)
            dwhbll::debug::panic("Malformed NormalizationTest.txt line \"{}\", @Part1 however len(c1 != 1)", cleaned_line);

        if (part1)
            individual_tested.insert(c1[0]);

        if (++total % 64 == 0)
            std::cout << "\n";

        auto [passed, infostr] = test_norm(c1, c2, c3, c4, c5);

        if (passed) {
            pass++;
            std::cout << "P";
        } else {
            dwhbll::console::info(infostr);
        }
    }

    std::cout << "\n";
    dwhbll::console::info("PASS: {}/{}, FAIL: {}/{}", pass, total, total - pass, total);
    dwhbll::console::info("OVERALL: PASS: {}/{}, FAIL: {}/{}", overall_pass, overall, overall - overall_pass, overall);

    return overall_pass != overall;
}
