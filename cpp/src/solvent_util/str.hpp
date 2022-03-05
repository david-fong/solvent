#ifndef HPP_SOLVENT_UTIL__STR
#define HPP_SOLVENT_UTIL__STR

#include "solvent_config.hpp"

#include <locale>
#include <array>
#include <string>

//
namespace solvent::util::str {

	struct MyNumPunct final : std::numpunct<char> {
		std::string do_grouping(void) const {
			return "\03";
		}
	};


	struct SgrPair final {
		char const*const on;
		char const*const off;
	};
	#if USE_ANSI_ESC
	#define SGR(NAME, ON_STR, OFF_STR) constexpr SgrPair NAME { .on {ON_STR}, .off {OFF_STR} };
	#else
	#define SGR(NAME, ON_STR, OFF_STR) constexpr SgrPair NAME { .on {""}, .off {""} };
	#endif

	SGR(dim, "\033[2m",  "\033[22m")
	SGR(red, "\033[31m", "\033[39m")

	#undef SGR


	/**
	 * Do not attempt to fill-initialize strings using entries from this
	 * array. The result will not be as expected, which may have to do
	 * with the use of utf-8 strings...
	 *
	 * NOTE: Make sure that the initializer list size matches that
	 * of the corresponding template argument. Compilers won't warn.
	 * See https://cppreference.com/w/cpp/language/sizeof...#Example
	 * for an example utility function I can make to avoid this problem.
	 */
	inline const std::array<std::string, 4> block_chars {
		#if USE_ANSI_ESC
		"\u2591", "\u2592", "\u2593", "\u2588",
		#else
		"-", "*", "X", "#",
		#endif
	};
}
#endif