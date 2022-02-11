#if defined(__APPLE__) && defined(__MACH__)

#	include <debugging.hpp>

#	include <algorithm>
#	include <array>

#	include <mach/mach_init.h>
#	include <mach/task.h>

template <class T>
auto exc = std::array<T, EXC_TYPES_COUNT> { {} };

namespace stdx {

namespace
{
	struct debugger_info
	{
		enum class is_present_state { unknown = 0, present, absent };

		std::atomic<is_present_state> is_present;

		debugger_info()
		{
			is_present.store(query_debugger_present());
		}

		static is_present_state query_debugger_present() noexcept
		{
			mach_msg_type_number_t count {};
			auto masks = exc<exception_mask_t>;
			auto ports = exc<mach_port_t>;
			auto behaviors = exc<exception_behavior_t>;
			auto flavors = exc<thread_state_flavor_t>;
			exception_mast_t mask
				= EXC_MASK_ALL & ~(EXC_MASK_RESOURCE | EXC_MASK_GUARD);
			kern_return_t result = task_get_exception_ports(mach_task_self(), mask,
				masks.data(), &count, ports.data(), behaviors.data(), flavors.data());
			if (result != KERN_SUCCESS)
			{
				return false;
			}

			auto valid = [](auto port) { return MACH_PORT_VALID(port); };
			auto begin = std::begin(ports);
			auto end = begin + count;
			return end != std::find_if(begin, end, valid)
				? is_present_state::present
				: is_present_state::absent;
		}
	};

	static debugger_info cached_debugger_info;
}

bool is_debugger_present(debugger_query q) noexcept
{
	if (q == debugger_query::immediate)
		cached_debugger_info.is_present.store(
			debugger_info::query_debugger_present());
	return cached_debugger_info.is_present.load()
		== debugger_info::is_present_state::present;
}

} // namespace stdx

#endif
