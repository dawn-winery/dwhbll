#include <dwhbll/cli/cli.h>
#include <utility>
#include <dwhbll/stl_ext/common_helpers.h>

using dwhbll::stl_ext::Option;

namespace dwhbll::cli {

Arg::Arg(std::string id) : id_(std::move(id)) {}

Arg::Arg(const Arg &other) {
    id_ = other.id_;
    help_ = other.help_;
    long_help_ = other.long_help_;
    short_opt_ = other.short_opt_;
    long_opt_ = other.long_opt_;
    long_aliases_ = other.long_aliases_;
    short_aliases_ = other.short_aliases_;
    action_ = other.action_;
    value_names_ = other.value_names_;
    num_args_ = other.num_args_;
    value_delimiter_ = other.value_delimiter_;
    default_values_ = other.default_values_;
    default_missing_values_ = other.default_missing_values_;
    default_value_ifs_ = other.default_value_ifs_;
    value_hint_ = other.value_hint_;
    index_ = other.index_;
    required_ = other.required_;
    global_ = other.global_;
    last_ = other.last_;
    trailing_var_arg_ = other.trailing_var_arg_;
    exclusive_ = other.exclusive_;
    ignore_case_ = other.ignore_case_;
    allow_hyphen_values_ = other.allow_hyphen_values_;
    allow_negative_numbers_ = other.allow_negative_numbers_;
    require_equals_ = other.require_equals_;
    raw_ = other.raw_;
    display_order_ = other.display_order_;
    next_line_help_ = other.next_line_help_;
    hide_short_help_ = other.hide_short_help_;
    hide_long_help_ = other.hide_long_help_;
    hide_possible_values_ = other.hide_possible_values_;
    hide_default_value_ = other.hide_default_value_;
    hide_env_ = other.hide_env_;
    hide_env_values_ = other.hide_env_values_;
    conflicts_ = other.conflicts_;
    requires_list_ = other.requires_list_;
    overrides_ = other.overrides_;
    groups_ = other.groups_;
    terminator_ = other.terminator_;
    help_heading_ = other.help_heading_;
    env_ = other.env_;
    env_default_ = other.env_default_;
    value_terminator_ = other.value_terminator_;
    if (other.value_parser_) {
        value_parser_ = other.value_parser_->clone();
    }
}

Arg &Arg::operator=(const Arg &other) {
    if (this != &other) {
        id_ = other.id_;
        help_ = other.help_;
        long_help_ = other.long_help_;
        short_opt_ = other.short_opt_;
        long_opt_ = other.long_opt_;
        long_aliases_ = other.long_aliases_;
        short_aliases_ = other.short_aliases_;
        action_ = other.action_;
        value_names_ = other.value_names_;
        num_args_ = other.num_args_;
        value_delimiter_ = other.value_delimiter_;
        default_values_ = other.default_values_;
        default_missing_values_ = other.default_missing_values_;
        default_value_ifs_ = other.default_value_ifs_;
        value_hint_ = other.value_hint_;
        index_ = other.index_;
        required_ = other.required_;
        global_ = other.global_;
        last_ = other.last_;
        trailing_var_arg_ = other.trailing_var_arg_;
        exclusive_ = other.exclusive_;
        ignore_case_ = other.ignore_case_;
        allow_hyphen_values_ = other.allow_hyphen_values_;
        allow_negative_numbers_ = other.allow_negative_numbers_;
        require_equals_ = other.require_equals_;
        raw_ = other.raw_;
        display_order_ = other.display_order_;
        next_line_help_ = other.next_line_help_;
        hide_short_help_ = other.hide_short_help_;
        hide_long_help_ = other.hide_long_help_;
        hide_possible_values_ = other.hide_possible_values_;
        hide_default_value_ = other.hide_default_value_;
        hide_env_ = other.hide_env_;
        hide_env_values_ = other.hide_env_values_;
        conflicts_ = other.conflicts_;
        requires_list_ = other.requires_list_;
        overrides_ = other.overrides_;
        groups_ = other.groups_;
        terminator_ = other.terminator_;
        help_heading_ = other.help_heading_;
        env_ = other.env_;
        env_default_ = other.env_default_;
        value_terminator_ = other.value_terminator_;
        if (other.value_parser_) {
            value_parser_ = other.value_parser_->clone();
        } else {
            value_parser_.reset();
        }
    }
    return *this;
}

Arg &Arg::id(std::string id) {
    id_ = std::move(id);
    return *this;
}

Arg &Arg::short_opt(char s) {
    short_opt_ = Option<char>(std::move(s));
    return *this;
}

Arg &Arg::long_opt(std::string l) {
    long_opt_ = Option<std::string>(std::move(l));
    return *this;
}

Arg &Arg::help(std::string h) {
    help_ = Option<std::string>(std::move(h));
    return *this;
}

Arg &Arg::long_help(std::string h) {
    long_help_ = Option<std::string>(std::move(h));
    return *this;
}

Arg &Arg::action(ArgAction a) {
    action_ = Option<ArgAction>(std::move(a));
    return *this;
}

Arg &Arg::value_name(std::string name) {
    value_names_.clear();
    value_names_.push_back(std::move(name));
    return *this;
}

Arg &Arg::value_names(std::initializer_list<std::string> names) {
    value_names_.clear();
    value_names_.insert(value_names_.end(), names);
    return *this;
}

Arg &Arg::num_args(ValueRange range) {
    num_args_ = Option<ValueRange>(std::move(range));
    return *this;
}

Arg &Arg::value_delimiter(char c) {
    value_delimiter_ = Option<char>(std::move(c));
    return *this;
}

Arg &Arg::default_value(std::string val) {
    default_values_.clear();
    default_values_.push_back(std::move(val));
    return *this;
}

Arg &Arg::default_values(std::initializer_list<std::string> vals) {
    default_values_.clear();
    default_values_.insert(default_values_.end(), vals);
    return *this;
}

Arg &Arg::default_missing_value(std::string val) {
    default_missing_values_.clear();
    default_missing_values_.push_back(std::move(val));
    return *this;
}

Arg &Arg::default_missing_values(std::initializer_list<std::string> vals) {
    default_missing_values_.clear();
    default_missing_values_.insert(default_missing_values_.end(), vals);
    return *this;
}

Arg &Arg::default_value_if(ArgPredicate pred, std::string val) {
    ConditionalDefault cd;
    cd.predicate = std::move(pred);
    cd.values = {std::move(val)};
    cd.is_if_all = false;
    cd.is_unless = false;
    default_value_ifs_.push_back(std::move(cd));
    return *this;
}

Arg &Arg::default_value_if_all(ArgPredicate pred, std::initializer_list<std::string> vals) {
    ConditionalDefault cd;
    cd.predicate = std::move(pred);
    cd.values = {vals};
    cd.is_if_all = true;
    cd.is_unless = false;
    default_value_ifs_.push_back(std::move(cd));
    return *this;
}

Arg &Arg::default_value_unless(ArgPredicate pred, std::string val) {
    ConditionalDefault cd;
    cd.predicate = std::move(pred);
    cd.values = {std::move(val)};
    cd.is_if_all = false;
    cd.is_unless = true;
    default_value_ifs_.push_back(std::move(cd));
    return *this;
}

Arg &Arg::value_hint(ValueHint hint) {
    value_hint_ = Option<ValueHint>(std::move(hint));
    return *this;
}

Arg &Arg::value_parser(std::unique_ptr<ValueParser> parser) {
    value_parser_ = std::move(parser);
    return *this;
}

Arg &Arg::value_parser(std::initializer_list<std::string> vals) {
    return value_parser(detail::make_value_parser(vals));
}

Arg &Arg::index(size_t idx) {
    index_ = Option<size_t>(std::move(idx));
    return *this;
}

Arg &Arg::required(bool yes) {
    required_ = yes;
    return *this;
}

Arg &Arg::global(bool yes) {
    global_ = yes;
    return *this;
}

Arg &Arg::last(bool yes) {
    last_ = yes;
    return *this;
}

Arg &Arg::trailing_var_arg(bool yes) {
    trailing_var_arg_ = yes;
    return *this;
}

Arg &Arg::exclusive(bool yes) {
    exclusive_ = yes;
    return *this;
}

Arg &Arg::ignore_case(bool yes) {
    ignore_case_ = yes;
    return *this;
}

Arg &Arg::allow_hyphen_values(bool yes) {
    allow_hyphen_values_ = yes;
    return *this;
}

Arg &Arg::allow_negative_numbers(bool yes) {
    allow_negative_numbers_ = yes;
    return *this;
}

Arg &Arg::require_equals(bool yes) {
    require_equals_ = yes;
    return *this;
}

Arg &Arg::value_terminator(std::string term) {
    value_terminator_ = Option<std::string>(std::move(term));
    return *this;
}

Arg &Arg::raw(bool yes) {
    raw_ = yes;
    if (yes) {
        allow_hyphen_values_ = true;
        last_ = true;
        if (!num_args_.is_some()) {
            num_args_ = Option<ValueRange>(ValueRange::at_least(1));
        }
    }
    return *this;
}

Arg &Arg::display_order(size_t order) {
    display_order_ = dwhbll::stl_ext::Some(order);
    return *this;
}

Arg &Arg::next_line_help(bool yes) {
    next_line_help_ = yes;
    return *this;
}

Arg &Arg::hide_short_help(bool yes) {
    hide_short_help_ = yes;
    return *this;
}

Arg &Arg::hide_long_help(bool yes) {
    hide_long_help_ = yes;
    return *this;
}

Arg &Arg::hide_possible_values(bool yes) {
    hide_possible_values_ = yes;
    return *this;
}

Arg &Arg::hide_default_value(bool yes) {
    hide_default_value_ = yes;
    return *this;
}

Arg &Arg::hide_env(bool yes) {
    hide_env_ = yes;
    return *this;
}

Arg &Arg::hide_env_values(bool yes) {
    hide_env_values_ = yes;
    return *this;
}

Arg &Arg::alias(std::string name, bool visible) {
    long_aliases_.emplace_back(std::move(name), visible);
    return *this;
}

Arg &Arg::aliases(std::initializer_list<std::string> names, bool visible) {
    for (const auto& name : names) {
        long_aliases_.emplace_back(name, visible);
    }
    return *this;
}

Arg &Arg::short_alias(char name, bool visible) {
    short_aliases_.emplace_back(name, visible);
    return *this;
}

Arg &Arg::short_aliases(std::initializer_list<char> names, bool visible) {
    for (const auto& name : names) {
        short_aliases_.emplace_back(name, visible);
    }
    return *this;
}

Arg &Arg::conflicts_with(std::string arg_id) {
    conflicts_.push_back(std::move(arg_id));
    return *this;
}

Arg &Arg::conflicts_with(std::initializer_list<std::string> arg_ids) {
    conflicts_.insert(conflicts_.end(), arg_ids);
    return *this;
}

Arg &Arg::requires_arg(std::string arg_id) {
    requires_list_.push_back(std::move(arg_id));
    return *this;
}

Arg &Arg::requires_arg(std::initializer_list<std::string> arg_ids) {
    requires_list_.insert(requires_list_.end(), arg_ids);
    return *this;
}

Arg &Arg::overrides_with(std::string arg_id) {
    overrides_.push_back(std::move(arg_id));
    return *this;
}

Arg &Arg::overrides_with(std::initializer_list<std::string> arg_ids) {
    overrides_.insert(overrides_.end(), arg_ids);
    return *this;
}

Arg &Arg::group(std::string group_id) {
    groups_.push_back(std::move(group_id));
    return *this;
}

Arg &Arg::groups(std::initializer_list<std::string> group_ids) {
    groups_.insert(groups_.end(), group_ids);
    return *this;
}

Arg &Arg::terminator(std::string term) {
    terminator_ = Option<std::string>(std::move(term));
    return *this;
}

Arg &Arg::help_heading(std::string heading) {
    help_heading_ = Option<std::string>(std::move(heading));
    return *this;
}

Arg &Arg::env(std::string var_name, dwhbll::stl_ext::Option<std::string> default_val) {
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

ArgGroup::ArgGroup(std::string id) : id_(std::move(id)) {}

ArgGroup &ArgGroup::id(std::string id) {
    id_ = std::move(id);
    return *this;
}

ArgGroup &ArgGroup::arg(std::string arg_id) {
    args_.push_back(std::move(arg_id));
    return *this;
}

ArgGroup &ArgGroup::args(std::initializer_list<std::string> arg_ids) {
    for (const auto& id : arg_ids) {
        args_.push_back(id);
    }
    return *this;
}

ArgGroup &ArgGroup::multiple(bool yes) {
    multiple_ = yes;
    return *this;
}

ArgGroup &ArgGroup::required(bool yes) {
    required_ = yes;
    return *this;
}

ArgGroup &ArgGroup::requires_arg(std::string arg_id) {
    requires_.push_back(std::move(arg_id));
    return *this;
}

ArgGroup &ArgGroup::requires_args(std::initializer_list<std::string> arg_ids) {
    for (const auto& id : arg_ids) {
        requires_.push_back(id);
    }
    return *this;
}

ArgGroup &ArgGroup::conflicts_with(std::string arg_id) {
    conflicts_.push_back(std::move(arg_id));
    return *this;
}

ArgGroup &ArgGroup::conflicts_with_all(std::initializer_list<std::string> arg_ids) {
    for (const auto& id : arg_ids) {
        conflicts_.push_back(id);
    }
    return *this;
}

} // namespace dwhbll::cli
