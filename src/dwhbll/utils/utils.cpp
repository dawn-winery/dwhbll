#include <dwhbll/utils/utils.hpp>

#include <ranges>

namespace dwhbll::utils {

constexpr std::string replace_all(std::string_view str, std::string_view from, std::string_view to) {
    return str
        | std::views::split(from) 
        | std::views::join_with(to) 
        | std::ranges::to<std::string>();
}

std::string escape_string(std::string_view str) {
    std::string s = utils::replace_all(str, "\"", "\\\"");
    return utils::replace_all(s, "\\", "\\\\");
}

} // namespace dwhbll::utils
