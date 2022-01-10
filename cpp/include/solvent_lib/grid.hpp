#ifndef HPP_SOLVENT_LIB__GRID
#define HPP_SOLVENT_LIB__GRID

#include <solvent_lib/size.hpp>

#include <vector>
#include <array>

namespace solvent::lib {

	template<Order O> using grid_vec_t = std::vector<typename size<O>::ord2_t>;
	template<Order O, typename T=size<O>::ord2_t> using grid_mtx_t = std::array<std::array<T, O*O>, O*O>;

	template<Order O> [[nodiscard]] grid_vec_t<O> grid_mtx2vec(const grid_mtx_t<O>&) noexcept;
	template<Order O> [[nodiscard]] grid_mtx_t<O> grid_vec2mtx(const grid_vec_t<O>&) noexcept;

	// Returns true if any cell in the same house contain the same value.
	// Can be used for incomplete grids.
	template<Order O> [[nodiscard, gnu::const]] bool is_grid_invalid(const grid_mtx_t<O>&) noexcept;

	template<Order O> [[nodiscard, gnu::const]] constexpr typename size<O>::ord2_t rmi2row(const typename size<O>::ord4_t index) noexcept { return index / (O*O); }
	template<Order O> [[nodiscard, gnu::const]] constexpr typename size<O>::ord2_t rmi2col(const typename size<O>::ord4_t index) noexcept { return index % (O*O); }
	template<Order O> [[nodiscard, gnu::const]] constexpr typename size<O>::ord2_t rmi2blk(const typename size<O>::ord2_t row, const typename size<O>::ord2_t col) noexcept {
		return ((row / O) * O) + (col / O);
	}
	template<Order O> [[nodiscard, gnu::const]]
	constexpr typename size<O>::ord2_t rmi2blk(const typename size<O>::ord4_t index) noexcept {
		return rmi2blk<O>(rmi2row<O>(index), rmi2col<O>(index));
	}

	template<Order O> [[nodiscard, gnu::const]]
	static constexpr bool cells_share_house(typename size<O>::ord4_t c1, typename size<O>::ord4_t c2) noexcept {
		return (rmi2row<O>(c1) == rmi2row<O>(c2))
			||  (rmi2col<O>(c1) == rmi2col<O>(c2))
			||  (rmi2blk<O>(c1) == rmi2blk<O>(c2));
	}
	// Note: the compiler optimizes the division/modulus pairs just fine.
	
	template<Order O>
	struct blk_mask_chutes_t {
		std::array<typename size<O>::O2_mask_fast_t, O> row, col;
	};
	template<Order O>
	constexpr blk_mask_chutes_t blk_mask_chutes = [](){
		blk_mask_chutes_t<O> _;
		for (unsigned chute = 0; chute < O; chute++) {
			for (unsigned i = 0; i < O; i++) {
				_.row[chute] |= 1 << ((O*chute) + i);
				_.col[chute] |= 1 << ((O*i) + chute);
		}	}
		return _;
	};


	#define SOLVENT_TEMPL_TEMPL(O_) \
		extern template grid_vec_t<O_> grid_mtx2vec<O_>(const grid_mtx_t<O_>&) noexcept; \
		extern template grid_mtx_t<O_> grid_vec2mtx<O_>(const grid_vec_t<O_>&) noexcept; \
		extern template [[gnu::const]] bool is_grid_invalid<O_>(const grid_mtx_t<O_>&) noexcept;
	SOLVENT_INSTANTIATE_ORDER_TEMPLATES
	#undef SOLVENT_TEMPL_TEMPL
}


extern template class std::vector<std::uint_fast8_t>;
#define SOLVENT_TEMPL_TEMPL(O_) \
	extern template class std::array<std::array<typename solvent::size<O_>::ord2_t, O_*O_>, O_*O_>;
SOLVENT_INSTANTIATE_ORDER_TEMPLATES
#undef SOLVENT_TEMPL_TEMPL
#endif