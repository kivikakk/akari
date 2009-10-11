#include <UserGates.hpp>

static char *getline(u32 in) {
	u32 cs = 8, n = 0;
	char *kbbuf = (char *)syscall_malloc(cs);

	while (true) {
		u32 incoming = syscall_readNode("system.io.keyboard", "input", in, kbbuf + n, 1);
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
	u32 stdin = (u32)-1;
	while (stdin == (u32)-1) {
		stdin = syscall_obtainNodeListener("system.io.keyboard", "input");
	}

	syscall_puts("\nstdin is 0x");
	syscall_putl(stdin, 16);
	syscall_puts(".\n");

	while (true) {
		char *l = getline(stdin);
		syscall_puts("got line (0x");
		syscall_putl((u32)l, 16);
		syscall_puts("): ");
		syscall_puts(l);
		syscall_putc('\n');
		syscall_free(l);
	}

	syscall_exit();
}

