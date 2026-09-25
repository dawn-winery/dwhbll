module;

#include <dwhbll/console/ansi_escape.h>
#include <dwhbll/console/logging.h>

export module dwhbll.console;

export namespace dwhbll::console {
    using dwhbll::console::Level;
    using dwhbll::console::setLevel;
    using dwhbll::console::setCerrLevel;
    using dwhbll::console::setWantColors;
    using dwhbll::console::log;
    using dwhbll::console::fatal;
    using dwhbll::console::critical;
    using dwhbll::console::error;
    using dwhbll::console::warn;
    using dwhbll::console::info;
    using dwhbll::console::debug;
    using dwhbll::console::trace;
    using dwhbll::console::log_filter;
    using dwhbll::console::censoring_log_filter;
    using dwhbll::console::addLogFilter;

    namespace ansi_escape {
        using dwhbll::console::ansi_escape::Graphics;
        using dwhbll::console::ansi_escape::make_color;
        using dwhbll::console::ansi_escape::make_rgb;
        using dwhbll::console::ansi_escape::make_graphic_escape;

        namespace color {
            using dwhbll::console::ansi_escape::color::reset;
            using dwhbll::console::ansi_escape::color::bold;
            using dwhbll::console::ansi_escape::color::dim;
            using dwhbll::console::ansi_escape::color::red;
            using dwhbll::console::ansi_escape::color::green;
            using dwhbll::console::ansi_escape::color::yellow;
            using dwhbll::console::ansi_escape::color::magenta;
            using dwhbll::console::ansi_escape::color::cyan;
        }
    }
}
