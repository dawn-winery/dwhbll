#include <concepts>
#include <type_traits>


namespace dwhbll::linalg {

	/**
	 * @brief Defines that a Matrix Element must be copyable, both normally
	 * and trivially, have a default constructor, have operator== defined,
	 * and have arithmetic and logical
	 */
	template <typename T>
	concept MatElt = std::is_trivially_copyable_v<T> &&
		std::regular<T> &&
		requires (T a, T b) {
			{ (a + b) }  	-> std::convertible_to<T>;
			{ (a - b) }  	-> std::convertible_to<T>;
			{ (a * b) }  	-> std::convertible_to<T>;
			{ (a / b) }  	-> std::convertible_to<T>;
			{ (a && b) } 	-> std::convertible_to<T>;
			{ (a || b) } 	-> std::convertible_to<T>;
			{ (-a) } 	 	-> std::convertible_to<T>;
			{ (a += b) } 	-> std::same_as<T&>;
			{ (a -= b) } 	-> std::same_as<T&>;
			{ (a *= b) } 	-> std::same_as<T&>;
		};
}
