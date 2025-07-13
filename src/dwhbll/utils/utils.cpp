#include <dwhbll/utils/utils.hpp>

#include <ranges>

namespace dwhbll::utils {

std::string replace_all(const std::string_view& str, const std::string_view& from, const std::string_view& to) {
    return str
        | std::views::split(from) 
        | std::views::join_with(to) 
        | std::ranges::to<std::string>();
}

} // namespace dwhbll::utils
