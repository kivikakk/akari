#include <POSIX.hpp>
#include <Console.hpp>
#include <Akari.hpp>
#include <debug.hpp>

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
			++s1, ++s2;
		}
		// One or both may be NUL.
		if (*s1 < *s2) return -1;
		if (*s1 > *s2) return 1;
		return 0;
	}

	char *strcpy(char *dest, const char *src) {
		char *orig = dest;
		while (*src)
			*dest++ = *src++;
		*dest = 0;
		return orig;
	}
}
