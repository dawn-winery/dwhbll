#pragma once

#include <iostream>
#include <string_view>

#include <dwhbll/lang/common.h>

namespace dwhbll::console::diags {
    enum class diag_level {
        FATAL_ERROR,
        ERROR,
        WARNING,
        STYLE,
        NOTE,
        HELP,
    };

    enum class trail_type {
        INCLUDE,
        EXPAND,
        INSTANTIATION,
        GENERATED_CODE
    };

    struct trail_frame {
        trail_type type;
        lang::span site; ///< Location in parent file where trail was emitted
        std::string descr; ///< Note for this frame.
    };

    struct diag_code {
        std::string lang_category;
        std::string name;
    };

    struct fix {
        lang::span replacement_area;
        std::string fix_note;
        std::string replacement;
    };

    struct annotation {
        lang::span annotation_range;
        std::string message;
    };

    struct diagnostic {
        diag_level level; ///< Diagnostic level
        diag_code code; ///< diagnostic
        std::string message;

        std::vector<trail_frame> trail{};
        std::vector<annotation> annotations{};
        std::vector<fix> suggested_fixes{};
        std::vector<diagnostic> nested_diagnostics{};
    };

    class diagnostics;

    /**
     * @brief Generates inclusion and other kinds of trails.
     */
    struct trail_tracker {
        struct scope {
            scope(trail_tracker &tracker, trail_type type, const lang::span &site, std::string descr);

            ~scope();

            scope(const scope &other) = delete;

            scope(scope &&other) noexcept
                : tracker(other.tracker), active(other.active) {
                other.active = false;
            }

            scope & operator=(const scope &other) = delete;

            scope & operator=(scope &&other) noexcept = delete;

        private:
            trail_tracker &tracker;
            bool active{true};
        };

        [[nodiscard]] scope add(trail_type type, const lang::span &site, std::string descr);

        void push_frame(trail_type type, const lang::span &site, std::string descr);

        void pop_frame();

        friend class diagnostics;

    private:
        std::vector<trail_frame> frames;
    };

    struct diagnostic_builder {
        diagnostic_builder(diagnostics& engine, diag_level lvl, diag_code code, std::string msg);

        ~diagnostic_builder();

        diagnostic_builder(const diagnostic_builder &other) = delete;

        diagnostic_builder(diagnostic_builder &&other) noexcept
            : engine(other.engine),
              in_flight(std::move(other.in_flight)),
              last_created(other.last_created),
              active(other.active) {
            other.active = false;
        }

        diagnostic_builder & operator=(const diagnostic_builder &other) = delete;

        diagnostic_builder & operator=(diagnostic_builder &&other) noexcept = delete;

        /**
         * @brief Adds an annotation to the most recently created diagnostic
         * @param spn Span for the annotation
         * @param msg Annotation message
         */
        diagnostic_builder & annotate(lang::span spn, std::string msg);

        /**
         * @brief Provide a fix
         * @param spn Range to replace
         * @param msg Message for this fix
         * @param replacement Data to replace at range
         */
        diagnostic_builder & fix(lang::span spn, std::string msg, std::string replacement);

        /**
         * @brief Create a new nested diagnostic
         * @param spn Range for this new diagnostic
         * @param msg Message for this diagnostic
         */
        diagnostic_builder & note(const lang::span &spn, std::string msg);

    private:
        diagnostics& engine;
        diagnostic in_flight;
        std::size_t last_created{std::numeric_limits<std::size_t>::max()};
        bool active{true};

        [[nodiscard]] diagnostic& get_last() {
            if (last_created == std::numeric_limits<std::size_t>::max())
                return in_flight;
            return in_flight.nested_diagnostics[last_created];
        }
    };

    class diagnostic_consumer {
    public:
        virtual ~diagnostic_consumer() = default;

        virtual void consume_diagnostic(files::filejar::file_mgr &mgr, diagnostic &diag) = 0;
    };

    /**
     * @brief Diagnostics engine.
     */
    class diagnostics {
        std::vector<diagnostic> diags;
        trail_tracker *tr{nullptr};

        friend struct diagnostic_builder;

        void add_diagnostic(diagnostic &&diag);

    public:
        diagnostics() = default;

        explicit diagnostics(trail_tracker* tracker);

        [[nodiscard]] diagnostic_builder report(diag_level lvl, diag_code code, std::string msg);

        [[nodiscard]] uint32_t size() const;

        [[nodiscard]] bool empty() const;

        /**
         * @brief Emit all currently stored diagnostics using this
         * @param mgr filejar for the sources
         * @param consumer diagnostic consumer
         */
        void emit_diagnostics(files::filejar::file_mgr &mgr, diagnostic_consumer* consumer);
    };

    /**
     * @brief Prints diagnostics in a compiler-style, source-aware format.
     *
     * The first annotation on a diagnostic is its primary location. Additional
     * annotations, notes, include trails, and suggested fixes are printed below
     * the diagnostic.
     */
    class console_printer final : public diagnostic_consumer {
        std::ostream& output;
        bool colors;

        void print_diagnostic(files::filejar::file_mgr &mgr, const diagnostic &diag,
            std::string_view prefix);

    public:
        explicit console_printer(std::ostream& output = std::cerr, bool colors = false)
            : output(output), colors(colors) {}

        void set_colors(bool enabled) {
            colors = enabled;
        }

        [[nodiscard]] bool wants_colors() const {
            return colors;
        }

        void consume_diagnostic(files::filejar::file_mgr &mgr, diagnostic &diag) override;
    };
}
