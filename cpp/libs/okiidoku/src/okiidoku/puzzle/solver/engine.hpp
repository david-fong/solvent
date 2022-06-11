#ifndef HPP_OKIIDOKU__PUZZLE__SOLVER__ENGINE
#define HPP_OKIIDOKU__PUZZLE__SOLVER__ENGINE

#include <okiidoku/puzzle/solver/found_queue.hpp>
#include <okiidoku/grid.hpp>
#include <okiidoku/o2_bit_arr.hpp>

#include <stack>
#include <vector>
#include <memory> // unique_ptr
#include <type_traits>

/**
This class is a primitive for building a primarily-deductive solver.
It uses a cell-major representation. See the design docs for more info.
It exposes a mechanism to use backtracking (guessing)- intended as a
 fallback after applying at least _some_ deductive techniques.

Currently it is private to the library and used to build the public
 FastSolver and VeryDeductiveSolver.

The contracts require dutiful care to follow. Ie. where it sees fit, it
 optimizes for the performance of its intended use-cases at the cost of
 the mental overhead of knowing and following the resulting contracts.

I imagine an average library _user_ would not be interested in such tinkering.
Of course, anyone can clone the repo and do such tinkering within it if they wish.

Examples of various ways this could be used:
- FastSolver: eagerly/immediately consumes found candidate eliminations
- VeryDeductive: "hoards"/accumulates found candidate eliminations until
	no more can possibly be found without consuming any.
- hypothetical "StupidSolver" purely uses the guess mechanism (_very_
   inefficient, but it would still be able to find all possible solutions)
  - note that the engine saves large restore-states for each guess based
     on the assumption that guesses shouldn't be used excessively.
*/
namespace okiidoku::mono::detail::solver {

	template<Order O> requires(is_order_compiled(O)) struct EngineImpl;
	template<Order O> requires(is_order_compiled(O)) class Engine;

	struct UnwindInfo;

	// usage: must be called immediately when a cell's candidate-symbol count
	//  changes to zero. call to prepare to find another solution.
	// contract: `engine.no_solutions_remain` returns `false`.
	template<Order O> requires(is_order_compiled(O))
	UnwindInfo unwind_one_stack_frame_of_(EngineImpl<O>&) noexcept;


	struct [[nodiscard]] UnwindInfo final {
		#define OKIIDOKU_FOR_COMPILED_O(O_) \
		friend UnwindInfo unwind_one_stack_frame_of_<O_>(EngineImpl<O_>&) noexcept;
		OKIIDOKU_INSTANTIATE_ORDER_TEMPLATES
		#undef OKIIDOKU_FOR_COMPILED_O
	private:
		explicit consteval UnwindInfo(bool did_unwind, bool did_unwind_root) noexcept:
			did_unwind_{did_unwind}, did_unwind_root_{did_unwind_root} {}
	public:
		[[nodiscard, gnu::pure]] bool did_unwind() const noexcept { return did_unwind_; }
		[[nodiscard, gnu::pure]] bool did_unwind_root() const noexcept { return did_unwind_root_; }
		UnwindInfo() = delete;
		static constexpr UnwindInfo make_no_unwind() noexcept { return UnwindInfo{false, false}; }
	private:
		static constexpr UnwindInfo make_did_unwind_guess() noexcept { return UnwindInfo{true, false}; }
		static constexpr UnwindInfo make_did_unwind_root() noexcept { return UnwindInfo{true, true}; }

		bool did_unwind_;
		bool did_unwind_root_;
	};


	template<Order O> requires(is_order_compiled(O)) class CandElimFind;
	template<Order O> requires(is_order_compiled(O)) class CandElimApplyImpl;

	template<Order O> requires(is_order_compiled(O))
	using CandsGrid = detail::Gridlike<O, O2BitArr<O>>;

	template<Order O> requires(is_order_compiled(O))
	struct Guess final {
		int_ts::o4xs_t<O> rmi;
		int_ts::o2xs_t<O> val;
	};


	// Defines all lower-level operations on engine internals. `Engine` wraps
	// it to enforce appropriate access control to the engine user and to the
	// candidate elimination find and apply operations.
	template<Order O> requires(is_order_compiled(O))
	struct EngineImpl {
		friend UnwindInfo unwind_one_stack_frame_of_<O>(EngineImpl<O>&) noexcept;
		using T = Ints<O>;
		using o2i_t = int_ts::o2i_t<O>;
		using val_t = int_ts::o2xs_t<O>;
		using rmi_t = int_ts::o4xs_t<O>;
		using o4i_t = int_ts::o4i_t<O>;

		struct HouseSubsets final {
			struct CellTag {
				rmi_t rmi;
				int_ts::o2is_t<O> count_cache;
			};
			std::array<CellTag, T::O2> cell_tags;
			O2BitArr<O> is_begin;
		};
		using houses_subsets_t = std::array<
			std::array<HouseSubsets, T::O2>,
			house_types.size()
		>;

		struct Frame final {
			o4i_t num_puzcells_remaining;
			CandsGrid<O> cells_cands;
			houses_subsets_t houses_subsets;
		};

		// TODO consider a different design: cells_cands_ and num_puzcells_remaining_ are just the top
		// entry of the guess_stack_. no_solutions_remain_ is implied when the guess stack size is zero.
		//  This would make the EngineImpl struct size small enough to probably justify no longer wrapping
		//   Engine with unique_ptr in the Solver classes.
		struct GuessStackFrame final {
			// do separate dynamic alloc for each `CandsGrid` to reduce resizing noise.
			std::unique_ptr<Frame> frame;
			Guess<O> guess;
			GuessStackFrame(const Frame& frame_, const Guess<O> guess_) noexcept:
				frame{std::make_unique<Frame>(frame_)},
				guess{guess_}
			{}
		};
		using guess_stack_t = std::vector<GuessStackFrame>;
		// using guess_stack_t = std::stack<GuessStackFrame/* , std::vector<GuessStackFrame> */>;
	public:
		EngineImpl() noexcept = default;

		// contract: none. puzzle can even blatantly break the one rule.
		void reinit_with_puzzle(const Grid<O>& puzzle) noexcept;

		// The user of the engine must respond to `get_next_solution` with `std::nullopt`
		// if this returns `true`.
		//
		// Note: All candidate elimination techniques have a contract that this returns `false`.
		// contract: All other non-const member functions require that this return `false`.
		[[nodiscard, gnu::pure]]
		bool no_solutions_remain() const noexcept { return no_solutions_remain_; }

		// the candidate elimination queue is processed in the order of insertion.
		[[nodiscard, gnu::pure]]
		bool has_queued_cand_elims() const noexcept { return !found_queues_.is_empty(); }


		[[nodiscard, gnu::pure]]
		auto get_num_puzcells_remaining() const noexcept { return frame_.num_puzcells_remaining; }

		// contract: `val` is currently one of _multiple_ candidate-symbols at `rmi`.
		// contract: only call when `has_queued_cand_elims` returns `false`. There
		//  is _never_ a good reason to make a guess when you have a deduction ready.
		//  See design docs for more discussion.
		void push_guess(Guess<O>) noexcept;

		[[nodiscard, gnu::pure]]
		std::size_t get_guess_stack_depth() const noexcept { return guess_stack_.size(); };

		[[nodiscard, gnu::pure]]
		std::uint_fast64_t get_total_guesses() const noexcept { return total_guesses_; };

		// contract: `no_solutions_remain` returns `false`.
		// contract: `get_num_puzcells_remaining` returns zero.
		// returns a filled grid that follows the one rule and contains all the puzzle's givens.
		[[nodiscard, gnu::pure]]
		Grid<O> build_solution_obj() const noexcept;


		[[nodiscard, gnu::pure]] const auto& cells_cands() const noexcept { return frame_.cells_cands; }
		[[nodiscard, gnu::pure]] auto& houses_subsets() noexcept { return frame_.houses_subsets; }
		[[nodiscard, gnu::pure]] auto& get_found_queues_() noexcept { return found_queues_; }
		[[nodiscard, gnu::pure]] const auto& get_guess_stack_() const noexcept { return guess_stack_; }


		// contract: `val` is currently one of _multiple_ candidate-symbols at `rmi`.
		// contract: no previous call in context of the current guess stack has been
		//  made with the same value of `rmi`.
		// post-condition: `val` is registered as the only candidate-symbol at `rmi`.
		void register_new_given_(rmi_t rmi, val_t val) noexcept;

		// The specified candidate-symbol is allowed to already be removed.
		UnwindInfo do_elim_remove_sym_(rmi_t rmi, val_t cand) noexcept;

		// The specified candidate-symbols are allowed to already be removed.
		UnwindInfo do_elim_remove_syms_(rmi_t rmi, const O2BitArr<O>& to_remove) noexcept;

		UnwindInfo do_elim_retain_syms_(rmi_t rmi, const O2BitArr<O>& to_retain) noexcept;

	private:
		[[nodiscard, gnu::pure]] auto& mut_cells_cands() noexcept { return frame_.cells_cands; }

		// The specified candidate-symbol is allowed to already be removed.
		template<class F> requires(std::is_invocable_v<F, O2BitArr<O>&>)
		UnwindInfo do_elim_generic_(rmi_t rmi, F elim_fn) noexcept;

		// contract: must be called immediately when a cell's candidate-symbol count _changes_ to one.
		// contract: (it follows that) no previous call in the context of the current
		//  guess stack has been made with the same value of `rmi`.
		// contract: (it follows that) the cell at `rmi` has exactly one candidate-symbol.
		// post-condition: decrements `num_puzcells_remaining`.
		void enqueue_cand_elims_for_new_cell_claim_sym_(rmi_t rmi) noexcept;

		void debug_print_cells_cands_() const noexcept;
		[[nodiscard, gnu::pure]] bool debug_check_correct_num_puzcells_remaining_() const noexcept;


		Frame frame_ {
			.num_puzcells_remaining {T::O4},
			.cells_cands {},
			.houses_subsets {},
		};

		FoundQueues<O> found_queues_ {};
		guess_stack_t guess_stack_ {};
		std::uint_fast64_t total_guesses_ {0};
		bool no_solutions_remain_ {true};
	};


	// Note: The current usage of inheritance (rather than composition) is _only_
	// done to reduce boilerplate in writing the delegating member functions.
	template<Order O> requires(is_order_compiled(O))
	class Engine final : private EngineImpl<O> {
		friend class CandElimFind<O>;  // the class wraps implementations that can only see what they need.
		friend class CandElimApply<O>;
		friend class CandElimApplyImpl<O>;
		using T = Ints<O>;
		using val_t = int_ts::o2xs_t<O>;
		using rmi_t = int_ts::o4xs_t<O>;
	public:
		Engine() noexcept = default;

		// please read the contracts for the referenced functions.
		using EngineImpl<O>::reinit_with_puzzle;
		using EngineImpl<O>::no_solutions_remain;
		using EngineImpl<O>::has_queued_cand_elims;
		using EngineImpl<O>::get_num_puzcells_remaining;
		using EngineImpl<O>::push_guess;
		using EngineImpl<O>::get_guess_stack_depth;
		using EngineImpl<O>::get_total_guesses;
		using EngineImpl<O>::build_solution_obj;

		// contract: `no_solutions_remain` returns `false`.
		UnwindInfo unwind_one_stack_frame() noexcept;

		using EngineImpl<O>::do_elim_remove_sym_;
	};


	#define OKIIDOKU_FOR_COMPILED_O(O_) \
		extern template struct EngineImpl<O_>; \
		extern template class Engine<O_>;
	OKIIDOKU_INSTANTIATE_ORDER_TEMPLATES
	#undef OKIIDOKU_FOR_COMPILED_O
}
#endif