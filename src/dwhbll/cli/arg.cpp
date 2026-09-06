#include <dwhbll/cli/cli.h>
#include <utility>

using dwhbll::stl_ext::Option;

namespace dwhbll::cli {

Arg::Arg(std::string id) : id_(std::move(id)) {}

Arg& Arg::id(std::string id) {
    id_ = std::move(id);
    return *this;
}

Arg& Arg::short_opt(char s) {
    short_opt_ = Option<char>(std::move(s));
    return *this;
}

Arg& Arg::long_opt(std::string l) {
    long_opt_ = Option<std::string>(std::move(l));
    return *this;
}

Arg& Arg::help(std::string h) {
    help_ = Option<std::string>(std::move(h));
    return *this;
}

Arg& Arg::long_help(std::string h) {
    long_help_ = Option<std::string>(std::move(h));
    return *this;
}

Arg& Arg::action(ArgAction a) {
    action_ = Option<ArgAction>(std::move(a));
    return *this;
}

Arg& Arg::value_name(std::string name) {
    value_names_.clear();
    value_names_.push_back(std::move(name));
    return *this;
}

Arg& Arg::value_names(std::initializer_list<std::string> names) {
    value_names_.clear();
    value_names_.insert(value_names_.end(), names);
    return *this;
}

Arg& Arg::num_args(ValueRange range) {
    num_args_ = Option<ValueRange>(std::move(range));
    return *this;
}

Arg& Arg::value_delimiter(char c) {
    value_delimiter_ = Option<char>(std::move(c));
    return *this;
}

Arg& Arg::default_value(std::string val) {
    default_values_.clear();
    default_values_.push_back(std::move(val));
    return *this;
}

Arg& Arg::default_values(std::initializer_list<std::string> vals) {
    default_values_.clear();
    default_values_.insert(default_values_.end(), vals);
    return *this;
}

Arg& Arg::default_missing_value(std::string val) {
    default_missing_values_.clear();
    default_missing_values_.push_back(std::move(val));
    return *this;
}

Arg& Arg::default_missing_values(std::initializer_list<std::string> vals) {
    default_missing_values_.clear();
    default_missing_values_.insert(default_missing_values_.end(), vals);
    return *this;
}

Arg& Arg::value_hint(ValueHint hint) {
    value_hint_ = Option<ValueHint>(std::move(hint));
    return *this;
}

Arg& Arg::index(size_t idx) {
    index_ = Option<size_t>(std::move(idx));
    return *this;
}

Arg& Arg::required(bool yes) {
    required_ = yes;
    return *this;
}

Arg& Arg::global(bool yes) {
    global_ = yes;
    return *this;
}

Arg& Arg::last(bool yes) {
    last_ = yes;
    return *this;
}

Arg& Arg::trailing_var_arg(bool yes) {
    trailing_var_arg_ = yes;
    return *this;
}

Arg& Arg::exclusive(bool yes) {
    exclusive_ = yes;
    return *this;
}

Arg& Arg::ignore_case(bool yes) {
    ignore_case_ = yes;
    return *this;
}

Arg& Arg::alias(std::string name, bool visible) {
    long_aliases_.emplace_back(std::move(name), visible);
    return *this;
}

Arg& Arg::aliases(std::initializer_list<std::string> names, bool visible) {
    for (const auto& name : names) {
        long_aliases_.emplace_back(name, visible);
    }
    return *this;
}

Arg& Arg::short_alias(char name, bool visible) {
    short_aliases_.emplace_back(name, visible);
    return *this;
}

Arg& Arg::short_aliases(std::initializer_list<char> names, bool visible) {
    for (const auto& name : names) {
        short_aliases_.emplace_back(name, visible);
    }
    return *this;
}

Arg& Arg::conflicts_with(std::string arg_id) {
    conflicts_.push_back(std::move(arg_id));
    return *this;
}

Arg& Arg::conflicts_with(std::initializer_list<std::string> arg_ids) {
    conflicts_.insert(conflicts_.end(), arg_ids);
    return *this;
}

Arg& Arg::requires_arg(std::string arg_id) {
    requires_list_.push_back(std::move(arg_id));
    return *this;
}

Arg& Arg::requires_arg(std::initializer_list<std::string> arg_ids) {
    requires_list_.insert(requires_list_.end(), arg_ids);
    return *this;
}

Arg& Arg::overrides_with(std::string arg_id) {
    overrides_.push_back(std::move(arg_id));
    return *this;
}

Arg& Arg::overrides_with(std::initializer_list<std::string> arg_ids) {
    overrides_.insert(overrides_.end(), arg_ids);
    return *this;
}

Arg& Arg::group(std::string group_id) {
    groups_.push_back(std::move(group_id));
    return *this;
}

Arg& Arg::groups(std::initializer_list<std::string> group_ids) {
    groups_.insert(groups_.end(), group_ids);
    return *this;
}

Arg& Arg::terminator(std::string term) {
    terminator_ = Option<std::string>(std::move(term));
    return *this;
}

Arg& Arg::help_heading(std::string heading) {
    help_heading_ = Option<std::string>(std::move(heading));
    return *this;
}

Arg& Arg::env(std::string var_name, dwhbll::stl_ext::Option<std::string> default_val) {
    env_ = Option<std::string>(std::move(var_name));
    env_default_ = std::move(default_val);
    return *this;
}

bool Arg::takes_values() const noexcept {
    if (action_.is_some()) {
        auto act = action_.unwrap();
        return act != ArgAction::SetTrue && act != ArgAction::SetFalse &&
               act != ArgAction::Count   && act != ArgAction::Help     &&
               act != ArgAction::Version && act != ArgAction::HelpLong &&
               act != ArgAction::VersionLong;
    }
    return num_args_.is_some() || !value_names_.empty() || !default_values_.empty();
}

bool Arg::is_positional() const noexcept {
    return !short_opt_.is_some() && !long_opt_.is_some();
}

bool Arg::is_flag() const noexcept {
    if (action_.is_some()) {
        auto act = action_.unwrap();
        return act == ArgAction::SetTrue || act == ArgAction::SetFalse ||
               act == ArgAction::Count   || act == ArgAction::Help     ||
               act == ArgAction::Version || act == ArgAction::HelpLong ||
               act == ArgAction::VersionLong;
    }
    return !takes_values();
}

bool Arg::is_required() const noexcept {
    return required_;
}

Arg arg_short_long(char s, std::string l) {
    std::string id = l;
    if (id.empty()) id = std::string(1, s);
    Arg arg(std::move(id));
    arg.short_opt(s).long_opt(std::move(l));
    return arg;
}

} // namespace dwhbll::cli
