#include <cstddef>
#include <cstdint>

import std;
import dwhbll.cli;
import dwhbll.stl_ext;
import dwhbll.sanify;

using dwhbll::stl_ext::Option;

namespace dwhbll::cli {

ArgMatches::ArgMatches(const ArgMatches& other) {
    values_ = other.values_;
    flags_ = other.flags_;
    for (const auto& [name, sub] : other.subcommands_) {
        if (sub) {
            subcommands_[name] = std::make_unique<ArgMatches>(*sub);
        }
    }
}

ArgMatches& ArgMatches::operator=(const ArgMatches& other) {
    if (this != &other) {
        values_ = other.values_;
        flags_ = other.flags_;
        subcommands_.clear();
        for (const auto& [name, sub] : other.subcommands_) {
            if (sub) {
                subcommands_[name] = std::make_unique<ArgMatches>(*sub);
            }
        }
    }
    return *this;
}

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
            } else if constexpr (std::is_same_v<T, u8>) {
                return Option<T>(static_cast<u8>(std::stoi(opt.unwrap())));
            } else if constexpr (std::is_same_v<T, u16>) {
                return Option<T>(static_cast<u16>(std::stoi(opt.unwrap())));
            } else if constexpr (std::is_same_v<T, u32>) {
                return Option<T>(static_cast<u32>(std::stoul(opt.unwrap())));
            } else if constexpr (std::is_same_v<T, u64>) {
                return Option<T>(std::stoull(opt.unwrap()));
            } else if constexpr (std::is_same_v<T, f32>) {
                return Option<T>(std::stof(opt.unwrap()));
            } else if constexpr (std::is_same_v<T, f64>) {
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
