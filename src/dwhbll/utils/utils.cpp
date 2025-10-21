#include <dwhbll/utils/utils.hpp>

#include <ranges>

namespace dwhbll::utils {

constexpr std::string replace_all(std::string_view str, std::string_view from, std::string_view to) {
    return str
        | std::views::split(from)
        | std::views::join_with(to)
        | std::ranges::to<std::string>();
}
constexpr std::vector<std::string> split(std::string_view str, std::string_view sep) {
    std::vector<std::string> strs;
    const std::string::size_type sep_size = sep.size();

    if (sep.empty()) {
        strs.emplace_back(str);
        return strs;
    }

    std::string::size_type start = 0, pos = 0;
    while ((pos = str.find(sep, start)) != std::string::npos) {
        strs.emplace_back(str.substr(start, pos - start));
        start = pos + sep_size;
    }
    strs.emplace_back(str.substr(start));

    return strs;
}

std::string escape_string(std::string_view str) {
    std::string s = utils::replace_all(str, "\"", "\\\"");
    return utils::replace_all(s, "\\", "\\\\");
}

} // namespace dwhbll::utils
