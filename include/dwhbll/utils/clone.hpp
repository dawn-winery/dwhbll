#pragma once

#include <concepts>
#include <memory>
#include <meta>
#include <vector>

#if __cpp_impl_reflection >= 202506L

namespace dwhbll::utils {
template <typename T>
  requires std::copy_constructible<T> && (!std::is_aggregate_v<T>) &&
           (!std::ranges::input_range<T>)
T clone(T const &value);

template <typename U, typename D>
  requires std::copy_constructible<D>
std::unique_ptr<U, D> clone(std::unique_ptr<U, D> const &value);

template <typename T>
  requires std::is_aggregate_v<T>
T clone(T const &value);

template <typename T>
concept push_back_cloneable_range =
    (!std::is_aggregate_v<T>) && std::ranges::input_range<T> &&
    std::default_initializable<T> && requires(T result) {
      result.push_back(std::declval<std::ranges::range_value_t<T>>());
    };

template <push_back_cloneable_range T> T clone(T const &value);

namespace clone_impl {

template <typename T, std::meta::info... Members>
T aggregate(T const &source) {
  return T{utils::clone(source.[:Members:])...};
}

template <typename T>
consteval auto get_clone_fn() -> T (*)(T const &) {
  constexpr auto ctx = std::meta::access_context::unchecked();
  std::vector<std::meta::info> args{^^T};
  for (auto member : std::meta::nonstatic_data_members_of(^^T, ctx)) {
    args.push_back(std::meta::reflect_constant(member));
  }
  auto specialization = std::meta::substitute(^^aggregate, args);
  return std::meta::extract<T (*)(T const &)>(specialization);
}

} // namespace clone_impl

template <typename T>
  requires std::copy_constructible<T> && (!std::is_aggregate_v<T>) &&
           (!std::ranges::input_range<T>)
T clone(T const &value) {
  return T(value);
}

template <typename U, typename D>
  requires std::copy_constructible<D>
std::unique_ptr<U, D> clone(std::unique_ptr<U, D> const &value) {
  if (!value) {
    return {nullptr, value.get_deleter()};
  }
  return {new U(utils::clone(*value)), value.get_deleter()};
}

template <typename T>
  requires std::is_aggregate_v<T>
T clone(T const &value) {
  return clone_impl::get_clone_fn<T>()(value);
}

template <push_back_cloneable_range T>
T clone(T const &value) {
  T result{};
  if constexpr (requires { result.reserve(value.size()); }) {
    result.reserve(value.size());
  }
  for (auto const &element : value) {
    result.push_back(utils::clone(element));
  }
  return result;
}
} // namespace dwhbll::utils

#else

#error "Your compiler is bad. Plz understand"

#endif
