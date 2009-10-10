#include <UserGates.hpp>

void ShellProcess() {
	//while(1) asm volatile("hlt");
	syscall_exit();
}

