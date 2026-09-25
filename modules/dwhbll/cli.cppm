module;

#include <dwhbll/cli/cli.h>
#include <dwhbll/cli/command.h>

export module dwhbll.cli;

export namespace dwhbll::cli {
    using dwhbll::cli::ArgAction;
    using dwhbll::cli::ValueHint;
    using dwhbll::cli::ValueParser;
    using dwhbll::cli::StringValueParser;
    using dwhbll::cli::IntValueParser;
    using dwhbll::cli::UIntValueParser;
    using dwhbll::cli::FloatValueParser;
    using dwhbll::cli::BoolValueParser;
    using dwhbll::cli::PossibleValuesParser;

    using dwhbll::cli::value_parser_string;
    using dwhbll::cli::value_parser_int;
    using dwhbll::cli::value_parser_uint;
    using dwhbll::cli::value_parser_float;
    using dwhbll::cli::value_parser_bool;
    using dwhbll::cli::value_parser;

    using dwhbll::cli::ValueRange;
    using dwhbll::cli::ArgPredicateType;
    using dwhbll::cli::ArgPredicate;
    using dwhbll::cli::ConditionalDefault;
    using dwhbll::cli::Arg;
    using dwhbll::cli::arg_short_long;
    using dwhbll::cli::ArgGroup;
    using dwhbll::cli::CommandSetting;
    using dwhbll::cli::ArgMatches;
    using dwhbll::cli::ParseResult;
    using dwhbll::cli::Command;

    namespace detail {
        using dwhbll::cli::detail::make_value_parser;
    }
}
