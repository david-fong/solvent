#include <okiidoku_cli/repl.hpp>
#include <okiidoku/shared_rng.hpp>

#include <okiidoku_cli_utils/console_setup.hpp>

#include <iostream>  // cout,
#include <iomanip>   // hex
#include <charconv>
#include <cstring>
#include <random>    // random_device,

// OKIIDOKU_DEFINE_MT19937_64

/**
ARGUMENTS
1: grid order (default defined in compiled_orders).
2: scramble random key (default: device random number).
*/
int main(const int argc, char const *const argv[]) {
	okiidoku::util::setup_console();

	const unsigned user_order {(argc > 1)
		? static_cast<unsigned>(std::stoi(argv[1]))
		: okiidoku::compiled_orders[0]
	};
	const auto srand_key {[&]() -> std::uint_fast64_t {
		if (argc > 2) {
			const auto cstr {argv[2]};
			std::uint_fast64_t parsed;
			if (std::from_chars(cstr, cstr+std::strlen(cstr), parsed, 16).ec == std::errc{}) {
				return parsed;
			} else {
				std::cerr << "\nfailed to parse rng seed argument (hex u64). using random_device.";
			}
		}
		return std::random_device()();
	}()};

	std::cout << "\nparsed arguments:"
	<< "\n- arg 1 (grid order) : " << user_order
	<< "\n- arg 2 (srand key)  : " << std::hex << srand_key // TODO.mid ugh. it's using my numpunct grouping.
	<< std::endl;

	okiidoku::SharedRng shared_rng;
	shared_rng.rng.seed(srand_key);

	okiidoku::cli::Repl repl {user_order, shared_rng};
	repl.start();

	std::cout << "\nbye bye!\n" << std::endl;
	return 0;
}
