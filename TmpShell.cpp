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

	char kbbuf[1024];

	while (true) {
		u32 incoming = syscall_readNode("system.io.keyboard", "input", listener, kbbuf, 1024);	// Will block.
		syscall_puts("Returned: value is ");
		syscall_putl(incoming, 10);
		syscall_puts("...?");

		if (!incoming) {
			//syscall_defer();
		} else {
			syscall_puts("Heard from kb: ");
			kbbuf[incoming] = 0;
			syscall_puts(kbbuf);
			syscall_putc('\n');
		}
	}

	syscall_exit();
}

