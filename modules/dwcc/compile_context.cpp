#include <dwcc/compile_context.h>

#include <dwhbll/lang/common/file_stream.h>
#include <dwhbll/lang/cpp/cpp_preprocessing_stream.h>
#include <dwhbll/lang/cpp/cpp_raw_stream.h>

namespace dwhbll::dwcc {
    void cached_lexer_data::invalidate() {
        lexed = false;
        tokens.clear();
        parsed = false;
    }

    files::filejar::fileid cctx::register_file(std::filesystem::path path) {
        files::filejar::fileid file = files.add_file(path);

        files.hash_file(file);

        if (cached_pp_files.contains(file)) {
            auto &cfile = cached_pp_files[file];
            if (cfile.metadata != files.metadata(file)) {
                cfile.invalidate();
            }
        } else {
            cached_pp_files[file] = {
                files.metadata(file),
                false,
                {},
                false,
            };
        }
    }

    const std::vector<lang::cpp::cpp_pp_token>& cctx::get_pp_tokens(const files::filejar::fileid &file) {
        auto it = cached_pp_files.find(file);
        if (it == cached_pp_files.end())
            debug::panic("File {:#x} was not registered with cctx!", file.id);

        auto& f = it->second;

        if (!f.lexed) {
            auto source_stream = std::make_unique<lang::common::file_stream> (
                files.contents(file)
            );

            auto raw_stream = std::make_unique<lang::cpp::cpp_raw_stream> (
                std::move(source_stream)
            );

            auto ppstream = std::make_unique<lang::cpp::cpp_preprocessing_stream>(
                file,
                std::move(raw_stream)
            );

            auto &test_stream = *ppstream;

            std::vector<lang::cpp::cpp_pp_token> tokens;

            for (auto tok : test_stream.generator)
                tokens.push_back(std::move(tok));

            f.lexed = true;
            f.tokens = std::move(tokens);
        }

        return f.tokens;
    }
}
