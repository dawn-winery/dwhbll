#include <concepts>
#include <vector>
#include <array>
#include <cstddef>
#include <mdspan>
#include <memory>
#include <span>
#include <type_traits>
#include "concepts.h"

namespace dwhbll::linalg {

using row_major = std::layout_right;
using col_major = std::layout_left;
constexpr size_t static_size_limit = 4096;	// If you need a single 4 KiB object on the stack, no, you need help

// Dimension helpers
template <size_t Dim> struct is_dim_static { static constexpr bool value = (Dim != std::dynamic_extent); };
template <size_t Dim> constexpr bool is_dim_static_v = is_dim_static<Dim>::value;

template <size_t Dim>
using dim_t = std::conditional_t<is_dim_static_v<Dim>, const size_t, size_t>;

/**
 * @brief Flat in-memory storage for a 2x2 matrix
 * @tparam T element type to store in the matrix, must satisfy the MatElt concept
 * @tparam Row the number of rows, default is dynamic
 * @tparam Col the number of colums, default is dynamic
 * @tparam Layout layout of the data in flat storage, default is row major (strides are rows)
 * @tparam Allocator allocator to use, default is std::allocator
 * @note If both dimensions are not dynamic and the default allocator is used and the requested
 * storage space is under 4 KiB, storage backing is std::array. Otherwise it is std::vector
 */
template <
	MatElt T,
	size_t Row = std::dynamic_extent,
	size_t Col = std::dynamic_extent,
	class Layout = row_major,
	class Allocator = std::allocator<T>
>
requires(Row != 0 && Col != 0)
class matrix {
	static constexpr bool is_fully_static = (is_dim_static_v<Row> && is_dim_static_v<Col>);
	static constexpr bool square_matrix = (Row == Col);
	static constexpr size_t static_size = is_fully_static ? Row * Col : 0;

	using value_type = T;
	using layout = Layout;
	using storage_t = std::conditional_t<
		is_fully_static && std::is_same_v<Allocator, std::allocator<T>> && static_size < static_size_limit,
		std::array<T, static_size>,
		std::vector<T, Allocator>
	>;

	storage_t storage;

	dim_t<Row> rows { is_dim_static_v<Row> ? Row : 0 };
	dim_t<Col> cols { is_dim_static_v<Col> ? Col : 0 };

public:
	matrix() = default;

	template <typename... Args>
	matrix(Args&&... args)
		requires(is_fully_static && sizeof...(Args) == (Row * Col) && (std::convertible_to<Args, T> && ...))
	{
		storage = { static_cast<T>(std::forward<Args>(args))... };
	}

	matrix(size_t r, size_t c)
		requires (Row == std::dynamic_extent && Col == std::dynamic_extent)
		: rows(r), cols(c)
	{
		storage.resize(r * c);
	}

	matrix(size_t r)
		requires(!is_fully_static && Row == std::dynamic_extent)
		: rows(r), cols(Col)
	{
		storage.resize(r * Col);
	}

	matrix(size_t c)
		requires(!is_fully_static && Col == std::dynamic_extent)
		: rows(Row), cols(c)
	{
		storage.resize(c * Row);
	}

	template <class Self>
	decltype(auto) operator[](this Self&& self, size_t r, size_t c)
	{
		assert(r < self.rows && c < self.cols);
		if constexpr (std::is_same_v<Layout, row_major>)
			return std::forward<Self>(self).storage[r * self.cols + c];
		else
			return std::forward<Self>(self).storage[c * self.rows + r];
	}

	size_t get_rows() const { return rows; }
	size_t get_cols() const { return cols; }
	size_t size() const { return storage.size(); }
	bool is_square()  const {
		if constexpr (is_fully_static) {
			return Row * Col;
		}
		return square_matrix;
	}
	template <class Self>
	auto data(this Self& self) { return self.storage.data(); }
};



template <
	MatElt T,
	size_t Row = std::dynamic_extent,
	size_t Col = std::dynamic_extent,
	class Allocator = std::allocator<T>
>
using col_matrix = matrix<T, Row, Col, col_major, Allocator>;


}
