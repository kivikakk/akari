#include <UserTask.hpp>
#include <Akari.hpp>

namespace User {
	u32 Task::GetProcessId() {
		return Akari->Task->current->_id;
	}
}

