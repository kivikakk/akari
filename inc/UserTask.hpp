#ifndef __USER_TASK_HPP__
#define __USER_TASK_HPP__

#include <arch.hpp>

namespace User {
	namespace Task {
		u32 GetProcessId();
		void IrqWait(u32 irq);
	}
}

#endif

