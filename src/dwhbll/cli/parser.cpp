#include <dwhbll/cli/cli.h>

namespace dwhbll::cli {

/* StringValueParser */

bool StringValueParser::parse(const std::string& input, std::string& output) const {
    output = input;
    return true;
}

std::string StringValueParser::type_name() const {
    return "string";
}

std::unique_ptr<ValueParser> StringValueParser::clone() const {
    return std::make_unique<StringValueParser>();
}

/* IntValueParser */

bool IntValueParser::parse(const std::string& input, std::string& output) const {
    try {
        std::stoi(input);
        output = input;
        return true;
    } catch (...) {
        return false;
    }
}

std::string IntValueParser::type_name() const {
    return "int";
}

std::unique_ptr<ValueParser> IntValueParser::clone() const {
    return std::make_unique<IntValueParser>();
}

/* UIntValueParser */

bool UIntValueParser::parse(const std::string& input, std::string& output) const {
    try {
        std::stoul(input);
        output = input;
        return true;
    } catch (...) {
        return false;
    }
}

std::string UIntValueParser::type_name() const {
    return "uint";
}

std::unique_ptr<ValueParser> UIntValueParser::clone() const {
    return std::make_unique<UIntValueParser>();
}

/* FloatValueParser */

bool FloatValueParser::parse(const std::string& input, std::string& output) const {
    try {
        std::stof(input);
        output = input;
        return true;
    } catch (...) {
        return false;
    }
}

std::string FloatValueParser::type_name() const {
    return "float";
}

std::unique_ptr<ValueParser> FloatValueParser::clone() const {
    return std::make_unique<FloatValueParser>();
}


/* BoolValueParser */

bool BoolValueParser::parse(const std::string& input, std::string& output) const {
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
        output = "true";
        return true;
    } else if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
        output = "false";
        return true;
    }
    return false;
}

std::string BoolValueParser::type_name() const {
    return "bool";
}

std::unique_ptr<ValueParser> BoolValueParser::clone() const {
    return std::make_unique<BoolValueParser>();
}

/* PossibleValuesParser */

PossibleValuesParser::PossibleValuesParser(std::initializer_list<std::string> vals) : values_(vals) {}

PossibleValuesParser::PossibleValuesParser(const std::vector<std::string>& vals) : values_(vals) {}

bool PossibleValuesParser::parse(const std::string& input, std::string& output) const {
    for (const auto& v : values_) {
        if (v == input) {
            output = input;
            return true;
        }
    }
    return false;
}

std::string PossibleValuesParser::type_name() const {
    return "string";
}

std::unique_ptr<ValueParser> PossibleValuesParser::clone() const {
    return std::make_unique<PossibleValuesParser>(values_);
}

} // namespace dwhbll::cli::cli
