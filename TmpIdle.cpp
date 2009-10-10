#include <UserGates.hpp>

void IdleProcess() {
	while(1) asm volatile("hlt");
}

