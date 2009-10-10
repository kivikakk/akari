#include <POSIX.hpp>
#include <Akari.hpp>

namespace POSIX {
	void *memset(void *mem, u8 c, u32 n) {
		u8 *m = (u8 *)mem;
		while (n--)
			*m++ = c;
		return mem;
	}

	void *memcpy(void *dest, const void *src, u32 n) {
		const u8 *s = (const u8 *)src;
		u8 *d = (u8 *)dest;
		while (n--)
			*d++ = *s++;
		return dest;
	}

	u32 strlen(const char *s) {
		u32 n = 0;
		while (*s++)
			++n;
		return n;
	}

	s32 strcmp(const char *s1, const char *s2) {
		while (*s1 && *s2) {
			if (*s1 < *s2) return -1;
			if (*s1 > *s2) return 1;
		}
		// One or both may be NUL.
		if (*s1 < *s2) return -1;
		if (*s1 > *s2) return 1;
		return 0;
	}
}
