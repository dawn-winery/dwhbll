#pragma once

#include <utility>

#include <dwhbll/console/diags/diagnostics.h>
#include <dwhbll/files/filejar/file.h>

namespace dwhbll::lang::cpp {
    struct cpp_pp_token;
}

namespace dwhbll::dwcc {
    struct cached_lexer_data {
        files::filejar::file_metadata metadata;
        bool lexed{false};
        std::vector<lang::cpp::cpp_pp_token> tokens;
        bool parsed{false};
        // TODO

        void invalidate();
    };

    /**
     * @brief Main compile context, global variables etc go here.
     */
    struct cctx {
        console::diags::trail_tracker trailgen{};
        console::diags::diagnostics diags{&trailgen};

        files::filejar::file_mgr files;
        std::unordered_map<files::filejar::fileid, cached_lexer_data> cached_pp_files;

        std::unordered_map<std::u32string, std::u32string> base_defines;

        cctx() = default;

        cctx(const cctx &other) = delete;

        cctx(cctx &&other) noexcept = delete;

        cctx & operator=(const cctx &other) = delete;

        cctx & operator=(cctx &&other) noexcept = delete;

        [[nodiscard]] constexpr console::diags::diagnostic_builder report_diag(
            const console::diags::diag_level lvl,
            console::diags::diag_code code, std::string msg) {
            return diags.report(lvl, std::move(code), std::move(msg));
        }

        files::filejar::fileid register_file(std::filesystem::path path);

        /**
         * @brief Special query providing preprocessor tokens
         * @param file the file
         * @return list of tokens
         */
        const std::vector<lang::cpp::cpp_pp_token>& get_pp_tokens(const files::filejar::fileid& file);
    };
}
