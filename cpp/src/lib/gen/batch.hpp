#ifndef HPP_SOLVENT_LIB_BATCH
#define HPP_SOLVENT_LIB_BATCH

#include "./mod.hpp"
#include ":/util/timer.hpp"

#include <mutex>
#include <string>
#include <vector>
#include <array>
#include <optional>

namespace solvent::lib::gen::batch {

	using trials_t = unsigned long;

	struct SharedData {
		trials_t total_anys;
		trials_t total_oks;

		struct MaxBacktrackSample {
			unsigned long max_backtracks;
			trials_t marginal_oks;
			double marginal_ops;
			double marginal_average_ops; // marginal_ops / marginal_oks
			double net_average_ops; // (accumulated marginal_ops) / (accumulated marginal_oks)
		};
		static constexpr unsigned SAMPLE_GRANULARITY_DEFAULT = 20u;
		static constexpr unsigned SAMPLE_GRANULARITY_MAX = 50u;

		// Data sampled. Each entry showing the outcome if its max_backtracks
		// value was used.
		std::vector<MaxBacktrackSample> max_backtrack_samples;
	};

	struct Params {
		gen::Params gen_params;
		unsigned num_threads = 0;
		unsigned max_backtrack_sample_granularity = SharedData::SAMPLE_GRANULARITY_DEFAULT;
		bool only_count_oks;
		trials_t stop_after;
	};

	//
	template<Order O>
	class ThreadFunc final {
	 static_assert(O > 0);
	 public:
		static constexpr unsigned NUM_EXTRA_THREADS = [](){
			if (O < 4) { return 0; }
			else if (O == 4) { return 1; }
			else { return 2; }
		}();
		static const unsigned DEFAULT_NUM_THREADS;

	 public:
		ThreadFunc(void) = delete;
		explicit ThreadFunc(
			const Params p, SharedData& sd, std::mutex& sdm,
			void(& grc)(const typename Generator<O>::GenResult)
		) : params_(p), shared_data_mutex_(sdm), shared_data_(sd), gen_result_consumer_(grc) {};

		void operator()();

		trials_t get_progress(void) const noexcept {
			if (params_.only_count_oks) {
				return shared_data_.total_oks;
			} else {
				return shared_data_.total_anys;
			}
		}

	 private:
		const Params params_;
		std::mutex& shared_data_mutex_;
		SharedData& shared_data_;
		void (& gen_result_consumer_)(const Generator<O>::GenResult);
		Generator<O> generator_;
	};

	//
	struct BatchReport : public SharedData {
		BatchReport() = delete;
		explicit BatchReport(SharedData shared_data, util::Timer::Elapsed time_elapsed)
			: SharedData(shared_data), time_elapsed(time_elapsed) {}

		util::Timer::Elapsed time_elapsed;
	};

	//
	template<Order O>
	const BatchReport batch(Params&, void(&)(const typename Generator<O>::GenResult&));
}
#endif