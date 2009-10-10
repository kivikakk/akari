#include <UserGates.hpp>

void ShellProcess() {
	u32 listener = (u32)-1;
	while (listener == (u32)-1) {
		listener = syscall_obtainNodeListener("system.io.keyboard", "input");
		syscall_putc('.');
	}

	syscall_puts("\nlistener is 0x");
	syscall_putl(listener, 16);
	syscall_puts(".\n");

	syscall_exit();
}

