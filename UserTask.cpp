#include <UserTask.hpp>
#include <Akari.hpp>

namespace User {
	u32 Task::GetProcessId() {
		return Akari->Task->current->_id;
	}

	void Task::IrqWait(u32 irq) {
		Akari->Task->current->irqWait = irq;
		Akari->Console->PutString("[<] ");
		irq0();	// do what the timer does. seems a fairly cheap (as in hackish) way to do things, though. スタックとか大丈夫かな
		Akari->Console->PutString("[>] ");
		Akari->Task->current->irqWait = 0;
	}
}

