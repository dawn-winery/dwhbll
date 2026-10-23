#include <concepts>
#include <initializer_list>
#include <vector>
#include <array>
#include <mdspan>
#include <memory>
#include <span>
#include <ranges>
#include <contracts>
#include <type_traits>
#include "../stl_ext/packs.h"
#include "concepts.h"

namespace dwhbll::linalg {


namespace __details {

	// Dimension helpers
	template <size_t Dim> struct is_dim_static { static constexpr bool value = (Dim != std::dynamic_extent); };
	template <size_t Dim> constexpr bool is_dim_static_v = is_dim_static<Dim>::value;

	enum class StaticDim {
		None = 0,
		Row  = 1,
		Col  = 2,
		Both = 3
	};

	template <size_t Row, size_t Col>
	struct static_dim_helper {
		static constexpr StaticDim value =
			(is_dim_static_v<Row> && is_dim_static_v<Col>)
				? StaticDim::Both
				: (!is_dim_static_v<Row> && is_dim_static_v<Col>)
					? StaticDim::Col
					: (is_dim_static_v<Row> && !is_dim_static_v<Col>)
						? StaticDim::Row
						: StaticDim::None;

	};

	template <size_t Row, size_t Col>
	static constexpr StaticDim which_dim_static = static_dim_helper<Row, Col>::value;


	///@brief Helper for chosing the type of the dimension members.
	/// If Dim is static, uses std::integral_constant to give it 0 size
	/// enabling it to be potentially overlapping and thus enabling move
	/// semantics and assignments despite being const
	template <size_t Dim>
	using dim_t = std::conditional_t<is_dim_static_v<Dim>, std::integral_constant<size_t, Dim>, size_t>;

	/**
	 * @brief Helper to ensure nested initializer lists are rectangular.
	 * Rectangularity means that all instances of the 2nd level of matrices
	 * are exactly the same size
     * @tparam T the type inside the nested lists
     * @param args the nested lists to verify
     * @return true if args is rectangular, false otherwise
	 */
	template <typename T>
	constexpr bool is_rectangular(std::initializer_list<std::initializer_list<T>> args) noexcept
	{
		const size_t control_size = args.begin()->size();
		for (auto&& c : args) {
			if (c.size() != control_size)
				return false;
		}
		return true;
	}

}


using row_major = std::layout_right;
using col_major = std::layout_left;
constexpr size_t static_size_limit = 4096;	// If you need a single 4 KiB object on the stack, no, you need help

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
	static constexpr bool is_fully_static 	= __details::which_dim_static<Row, Col> == __details::StaticDim::Both;
	static constexpr size_t static_size		= is_fully_static ? Row * Col : 0;

	using value_type = T;
	using layout = Layout;
	using storage_t = std::conditional_t<
		is_fully_static && std::is_same_v<Allocator, std::allocator<T>> && static_size < static_size_limit,
		std::array<T, static_size>,
		std::vector<T, Allocator>
	>;

	[[no_unique_address]] __details::dim_t<Row> rows {};
	[[no_unique_address]] __details::dim_t<Col> cols {};

	storage_t storage;

public:
	/**
	 * @brief Default constructor for constructing with nothing
	 * @note If the matrix has a std::array storage backing, the rows,
	 *		 cols and storage members are initialised. Otherwise they
     *		 are empty
	 */
	matrix() = default;

	/**
	 * @brief Constructor to 0 init a fully dynamic matrix with given size
	 * @param[size_t] r the initial number of rows
	 * @param[size_t] c the initial number of columns
	 * @note Both the Row and Col template parameters of the class must be
	 * 		 std::dynamic_extent
	 */
	matrix(size_t r, size_t c)
		requires (__details::which_dim_static<Row, Col> == __details::StaticDim::None)
		: rows(r), cols(c), storage(r * c)
	{
	}

	/**
	 * @brief Constructor to 0 init a half static matrix along its columns
	 * @param[size_t] r the initial number of rows
	 */
	matrix(size_t r)
		requires(__details::which_dim_static<Row, Col> == __details::StaticDim::Col)
		: rows(r), storage(r * Col)
	{
	}

	/**
	 * @brief Constructor to 0 init a half static matrix along its rows
	 * @param[size_t] r the initial number of columns
	 */
	matrix(size_t c)
		requires(__details::which_dim_static<Row, Col> == __details::StaticDim::Row)
		: cols(c), storage(c * Row)
	{
	}

	/**
	 * @brief Constructor with default values for matrices with at least one static dimension.
	 * Accepts a list of elements to insert inside the storage as a variadic pack.
	 * @tparam Args the variadic list of type of the inserted elements. Inferred by the compiler
	 * @param[Args...] args the elements to insert
	 * @note All inserted elements must be convertible to internal type of the matrix
	 * @note The amount of inserted element must be divisible by the static matrix to ensure a rectangular matrix
	 */
	template<MatElt... Args>
	matrix(Args&&... args)
		requires((sizeof...(Args) > 0 && (std::convertible_to<Args, T> && ...))
			&& ((is_fully_static && sizeof...(Args) == (Row * Col))
			|| (__details::is_dim_static_v<Row> && stl_ext::pack_cnt_divisible_by_v<Row, Args...>)
			|| (__details::is_dim_static_v<Col> && stl_ext::pack_cnt_divisible_by_v<Col, Args...>))
		)
		: storage { static_cast<T>(std::forward<Args>(args))... }
	{
		if constexpr (!__details::is_dim_static_v<Row>)
			rows = (sizeof...(Args) / Col);
		if constexpr (!__details::is_dim_static_v<Col>)
			cols = (sizeof...(Args) / Row);
	}

	/**
	 * @brief Constructor with default values for fully dynamic matrices.
	 * Uses a nested initializer_list of elements to give a hint for the dimensions.
	 * @param args the elements to insert
	 * @note Args must not be empty
	 * @note Args must be rectangular, meaning all instances of the 2nd dimensions must
	 * 		 be of the same size
	 */
	matrix(std::initializer_list<std::initializer_list<T>> args)
		#if __cpp_contracts >= 202502L
		pre(args.size() > 0 && __details::is_rectangular(args))
		#endif
		: rows(std::is_same_v<Layout, row_major> ? args.size() : args.begin()->size()),
		  cols(std::is_same_v<Layout, col_major> ? args.size() : args.begin()->size()),
		  storage(std::from_range, args | std::views::join)
	{
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
		if constexpr (is_fully_static)
			return Row == Col;
		else {
			if constexpr (__details::is_dim_static_v<Row>)
				return Row == cols;
			else if constexpr (__details::is_dim_static_v<Col>)
				return rows == Col;
			else
				return rows == cols;
		}
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
