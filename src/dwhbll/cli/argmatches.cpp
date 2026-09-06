#include <dwhbll/cli/command.h>

using dwhbll::stl_ext::Option;

namespace dwhbll::cli {

void ArgMatches::insert_value(std::string id, std::string value) {
    values_[std::move(id)].push_back(std::move(value));
}

void ArgMatches::insert_flag(std::string id, bool value) {
    flags_[std::move(id)] = value;
}

[[nodiscard]] Option<std::string> ArgMatches::get_one(const std::string& id) const {
    auto it = values_.find(id);
    if (it != values_.end() && !it->second.empty()) {
        return Option<std::string>(std::string(it->second.front()));
    }
    return Option<std::string>();
}

[[nodiscard]] std::vector<std::string> ArgMatches::get_many(const std::string& id) const {
    auto it = values_.find(id);
    if (it != values_.end()) {
        return it->second;
    }
    return {};
}

[[nodiscard]] bool ArgMatches::get_flag(const std::string& id) const {
    auto it = flags_.find(id);
    if (it != flags_.end()) {
        return it->second;
    }
    return false;
}

[[nodiscard]] bool ArgMatches::contains_id(const std::string& id) const {
    return values_.contains(id) || flags_.contains(id);
}

[[nodiscard]] size_t ArgMatches::count(const std::string& id) const {
    auto it = values_.find(id);
    if (it != values_.end()) {
        return it->second.size();
    }
    return 0;
}

void ArgMatches::insert_subcommand(std::string name, std::unique_ptr<ArgMatches> sub_matches) {
    subcommands_[std::move(name)] = std::move(sub_matches);
}

void ArgMatches::clear_values(const std::string& id) {
    values_.erase(id);
}

[[nodiscard]] dwhbll::stl_ext::Option<std::string> ArgMatches::subcommand_name() const {
    if (!subcommands_.empty()) {
        return dwhbll::stl_ext::Option<std::string>(std::string(subcommands_.begin()->first));
    }
    return dwhbll::stl_ext::Option<std::string>();
}

[[nodiscard]] const ArgMatches* ArgMatches::subcommand_matches(const std::string& name) const {
    auto it = subcommands_.find(name);
    if (it != subcommands_.end()) {
        return it->second.get();
    }
    return nullptr;
}

template<typename T>
[[nodiscard]] Option<T> ArgMatches::get_one_as(const std::string& id) const {
    auto opt = get_one(id);
    if (opt.is_some()) {
        try {
            if constexpr (std::is_same_v<T, int>) {
                return Option<T>(std::stoi(opt.unwrap()));
            } else if constexpr (std::is_same_v<T, uint8_t>) {
                return Option<T>(static_cast<uint8_t>(std::stoi(opt.unwrap())));
            } else if constexpr (std::is_same_v<T, uint16_t>) {
                return Option<T>(static_cast<uint16_t>(std::stoi(opt.unwrap())));
            } else if constexpr (std::is_same_v<T, uint32_t>) {
                return Option<T>(static_cast<uint32_t>(std::stoul(opt.unwrap())));
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                return Option<T>(std::stoull(opt.unwrap()));
            } else if constexpr (std::is_same_v<T, float>) {
                return Option<T>(std::stof(opt.unwrap()));
            } else if constexpr (std::is_same_v<T, double>) {
                return Option<T>(std::stod(opt.unwrap()));
            } else if constexpr (std::is_same_v<T, bool>) {
                return Option<T>(opt.unwrap() == "true" || opt.unwrap() == "1");
            } else if constexpr (std::is_same_v<T, std::string>) {
                return Option<T>(opt.unwrap());
            }
        } catch (...) {
            return Option<T>();
        }
    }
    return Option<T>();
}

} // namespace dwhbll::cli
