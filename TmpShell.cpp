#include <UserGates.hpp>

static u32 stdin;
static char *getline() {
	u32 cs = 8, n = 0;
	char *kbbuf = (char *)syscall_malloc(cs);

	while (true) {
		u32 incoming = syscall_readNode("system.io.keyboard", "input", stdin, kbbuf + n, 1);
		syscall_putc(kbbuf[n]);
		if (kbbuf[n] == '\n') break;

		n += incoming;	// 1

		if (n == cs) {
			char *nkb = (char *)syscall_malloc(cs * 2);
			syscall_memcpy(nkb, kbbuf, n);
			syscall_free(kbbuf);
			kbbuf = nkb;
			cs *= 2;
		}
	}

	kbbuf[n] = 0;
	return kbbuf;
}


void ShellProcess() {
	stdin = (u32)-1;
	while (stdin == (u32)-1) {
		stdin = syscall_obtainNodeListener("system.io.keyboard", "input");
		syscall_putc('.');
	}

	syscall_puts("\nstdin is 0x");
	syscall_putl(stdin, 16);
	syscall_puts(".\n");

	char kbbuf[1024];

	while (true) {
		u32 incoming = syscall_readNode("system.io.keyboard", "input", stdin, kbbuf, 1024);	// Will block.
		// syscall_puts("Returned: value is ");
		// syscall_putl(incoming, 10);
		// syscall_puts("\n");

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

