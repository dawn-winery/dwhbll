#include <dwhbll/console/diags/diagnostics.h>

#include <algorithm>
#include <format>
#include <string_view>
#include <utility>

namespace dwhbll::console::diags {
    trail_tracker::scope::scope(trail_tracker &tracker, const trail_type type,
        const lang::span &site, std::string descr) : tracker(tracker) {
        tracker.push_frame(type, site, std::move(descr));
    }

    trail_tracker::scope::~scope() {
        if (active)
            tracker.pop_frame();
    }

    trail_tracker::scope trail_tracker::add(const trail_type type,
        const lang::span &site, std::string descr) {
        return scope{*this, type, site, std::move(descr)};
    }

    void trail_tracker::push_frame(trail_type type, const lang::span &site,
        std::string descr) {
        frames.emplace_back(type, site, std::move(descr));
    }

    void trail_tracker::pop_frame() {
        frames.pop_back();
    }

    diagnostic_builder::diagnostic_builder(diagnostics &engine, const diag_level lvl,
        diag_code code, std::string msg) : engine(engine),
            in_flight(lvl, std::move(code), std::move(msg)) {}

    diagnostic_builder::~diagnostic_builder() {
        if (active)
            engine.add_diagnostic(std::move(in_flight));
    }

    diagnostic_builder & diagnostic_builder::annotate(lang::span spn,
        std::string msg) {
        get_last().annotations.emplace_back(spn, std::move(msg));

        return *this;
    }

    diagnostic_builder & diagnostic_builder::fix(lang::span spn,
        std::string msg, std::string replacement) {
        in_flight.suggested_fixes.emplace_back(spn, std::move(msg), std::move(replacement));

        return *this;
    }

    diagnostic_builder & diagnostic_builder::note(const lang::span &spn,
        std::string msg) {
        in_flight.nested_diagnostics.push_back(diagnostic{
            .level = diag_level::NOTE,
            .code = {},
            .message = std::move(msg),
            .annotations = {annotation{spn, ""}}
        });

        last_created = in_flight.nested_diagnostics.size() - 1;

        return *this;
    }

    void diagnostics::add_diagnostic(diagnostic &&diag) {
        diags.emplace_back(std::move(diag));
        if (tr)
            diags.back().trail = tr->frames;
    }

    diagnostics::diagnostics(trail_tracker *tracker) : tr(tracker) {
    }

    diagnostic_builder diagnostics::report(const diag_level lvl, diag_code code,
        std::string msg) {
        return diagnostic_builder{*this, lvl, std::move(code), std::move(msg)};
    }

    uint32_t diagnostics::size() const {
        return diags.size();
    }

    bool diagnostics::empty() const {
        return diags.empty();
    }

    void diagnostics::emit_diagnostics(files::filejar::file_mgr &mgr, diagnostic_consumer* consumer) {
        if (consumer == nullptr)
            return;

        for (auto& diag : diags) {
            consumer->consume_diagnostic(mgr, diag);
        }
    }

    namespace {
        constexpr std::string_view reset = "\033[0m";
        constexpr std::string_view red = "\033[1;31m";
        constexpr std::string_view yellow = "\033[1;33m";
        constexpr std::string_view cyan = "\033[1;36m";
        constexpr std::string_view green = "\033[1;32m";
        constexpr std::string_view blue = "\033[1;34m";

        std::string_view level_name(const diag_level level) {
            switch (level) {
            case diag_level::FATAL_ERROR: return "fatal error";
            case diag_level::ERROR: return "error";
            case diag_level::WARNING: return "warning";
            case diag_level::STYLE: return "style";
            case diag_level::NOTE: return "note";
            case diag_level::HELP: return "help";
            }
            return "diagnostic";
        }

        std::string_view level_color(const diag_level level) {
            switch (level) {
            case diag_level::FATAL_ERROR:
            case diag_level::ERROR: return red;
            case diag_level::WARNING:
            case diag_level::STYLE: return yellow;
            case diag_level::NOTE: return cyan;
            case diag_level::HELP: return green;
            }
            return {};
        }

        std::string source_line(const std::span<sanify::u8> contents, const std::size_t line) {
            if (line == 0)
                return {};

            std::size_t current_line = 1;
            std::size_t start = 0;
            for (std::size_t i = 0; i < contents.size(); ++i) {
                if (contents[i] != '\n')
                    continue;
                if (current_line == line)
                    return std::string(reinterpret_cast<const char*>(contents.data() + start), i - start);
                ++current_line;
                start = i + 1;
            }
            if (current_line == line)
                return std::string(reinterpret_cast<const char*>(contents.data() + start), contents.size() - start);
            return {};
        }

        std::size_t digit_count(std::size_t value) {
            std::size_t digits = 1;
            while (value >= 10) {
                value /= 10;
                ++digits;
            }
            return digits;
        }

        std::string numbered_prefix(const std::size_t line, const std::size_t width,
            const std::string_view marker = {}) {
            return std::format("{:>{}}{} | ", line, width, marker);
        }

        std::string location(const files::filejar::file_mgr &mgr, const lang::span &span) {
            return std::format("{}:{}:{}", mgr.path(span.file).string(),
                span.line_begin, span.column_begin);
        }

        std::string_view trail_name(const trail_type type) {
            switch (type) {
            case trail_type::INCLUDE: return "included from";
            case trail_type::EXPAND: return "expanded from";
            case trail_type::INSTANTIATION: return "instantiated from";
            case trail_type::GENERATED_CODE: return "generated from";
            }
            return "originated from";
        }

        std::vector<std::string> split_lines(std::string text) {
            std::vector<std::string> lines;
            std::size_t start = 0;
            while (start <= text.size()) {
                const auto end = text.find('\n', start);
                lines.emplace_back(text.substr(start, end == std::string::npos
                    ? std::string::npos : end - start));
                if (end == std::string::npos)
                    break;
                start = end + 1;
            }
            return lines;
        }

        std::vector<std::string> fix_lines(const std::span<sanify::u8> contents,
            const fix &suggestion, bool replaced) {
            std::vector<std::string> source_lines;
            std::size_t start = 0;
            for (std::size_t i = 0; i <= contents.size(); ++i) {
                if (i != contents.size() && contents[i] != '\n')
                    continue;
                source_lines.emplace_back(reinterpret_cast<const char*>(contents.data() + start), i - start);
                start = i + 1;
            }

            const auto begin_line = suggestion.replacement_area.line_begin;
            const auto end_line = suggestion.replacement_area.line_end;
            if (begin_line == 0 || end_line < begin_line ||
                end_line > source_lines.size())
                return {};
            if (!replaced)
                return {source_lines.begin() + begin_line - 1,
                    source_lines.begin() + end_line};

            const auto begin = suggestion.replacement_area.column_begin > 0
                ? suggestion.replacement_area.column_begin - 1 : 0;
            const auto end = suggestion.replacement_area.column_end > 0
                ? suggestion.replacement_area.column_end - 1 : 0;
            auto &first = source_lines[begin_line - 1];
            auto &last = source_lines[end_line - 1];
            if (begin > first.size() || end > last.size())
                return {};

            std::string replacement = first.substr(0, begin) + suggestion.replacement +
                last.substr(end);
            return split_lines(std::move(replacement));
        }
    }

    void console_printer::consume_diagnostic(files::filejar::file_mgr &mgr, diagnostic &diag) {
        print_diagnostic(mgr, diag, {});
    }

    void console_printer::print_diagnostic(files::filejar::file_mgr &mgr,
        const diagnostic &diag, const std::string_view prefix) {
        const annotation *primary = diag.annotations.empty() ? nullptr : &diag.annotations.front();
        const auto color = colors ? level_color(diag.level) : std::string_view{};
        const auto level = level_name(diag.level);

        output << prefix;
        if (primary != nullptr)
            output << location(mgr, primary->annotation_range) << ": ";
        output << color << level << (colors ? reset : "") << ": ";
        if (!diag.code.lang_category.empty() || !diag.code.name.empty())
            output << '[' << diag.code.lang_category
                   << (!diag.code.lang_category.empty() && !diag.code.name.empty() ? ":" : "")
                   << diag.code.name << "] ";
        output << diag.message << '\n';

        if (primary != nullptr && mgr.exists(primary->annotation_range.file)) {
            const auto contents = mgr.contents(primary->annotation_range.file);
            const auto line = source_line(contents, primary->annotation_range.line_begin);
            const auto width = digit_count(primary->annotation_range.line_begin);
            output << prefix << numbered_prefix(primary->annotation_range.line_begin, width)
                   << line << '\n';
            output << prefix << std::string(width, ' ') << " | "
                   << std::string(primary->annotation_range.column_begin > 0
                       ? primary->annotation_range.column_begin - 1 : 0, ' ')
                   << (colors ? blue : std::string_view{})
                   << '^';
            const auto end_column = primary->annotation_range.line_end ==
                primary->annotation_range.line_begin
                ? primary->annotation_range.column_end : primary->annotation_range.column_begin + 1;
            if (end_column > primary->annotation_range.column_begin + 1)
                output << std::string(end_column - primary->annotation_range.column_begin - 1, '~');
            output << (colors ? reset : std::string_view{});
            if (!primary->message.empty())
                output << ' ' << primary->message;
            output << '\n';
        }

        for (std::size_t i = primary == nullptr ? 0 : 1; i < diag.annotations.size(); ++i) {
            const auto &annotation = diag.annotations[i];
            output << prefix << "note: " << location(mgr, annotation.annotation_range);
            if (!annotation.message.empty())
                output << ": " << annotation.message;
            output << '\n';
        }

        for (const auto &trail : diag.trail) {
            output << prefix << "note: " << trail_name(trail.type);
            if (!trail.descr.empty())
                output << ": " << trail.descr;
            if (mgr.exists(trail.site.file))
                output << " at " << location(mgr, trail.site);
            output << '\n';
        }

        for (const auto &fix : diag.suggested_fixes) {
            output << prefix << "help: " << fix.fix_note << " (replace "
                   << location(mgr, fix.replacement_area) << " with `"
                   << fix.replacement << "`)\n";
            if (!mgr.exists(fix.replacement_area.file))
                continue;

            const auto contents = mgr.contents(fix.replacement_area.file);
            const auto old_lines = fix_lines(contents, fix, false);
            const auto new_lines = fix_lines(contents, fix, true);
            if (old_lines.empty() || new_lines.empty() || old_lines == new_lines)
                continue;

            const auto old_color = colors ? red : std::string_view{};
            const auto new_color = colors ? green : std::string_view{};
            const auto line_width = digit_count(std::max(
                fix.replacement_area.line_end,
                fix.replacement_area.line_begin + new_lines.size() - 1));
            for (std::size_t i = 0; i < old_lines.size(); ++i)
                output << prefix << old_color
                       << numbered_prefix(fix.replacement_area.line_begin + i, line_width, "-")
                       << old_lines[i]
                       << (colors ? reset : "") << '\n';
            for (std::size_t i = 0; i < new_lines.size(); ++i)
                output << prefix << new_color
                       << numbered_prefix(fix.replacement_area.line_begin + i, line_width, "+")
                       << new_lines[i]
                       << (colors ? reset : "") << '\n';
            output << prefix << std::string(line_width, ' ') << "  | "
                   << std::string(fix.replacement_area.column_begin > 0
                       ? fix.replacement_area.column_begin - 1 : 0, ' ')
                   << (colors ? blue : std::string_view{})
                   << std::string(std::max<std::size_t>(1, fix.replacement.size()), '^')
                   << (colors ? reset : "") << '\n';
        }

        for (const auto &nested_diag : diag.nested_diagnostics)
            print_diagnostic(mgr, nested_diag, std::string(prefix) + "  ");
    }
}
