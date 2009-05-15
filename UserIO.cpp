#include <UserIO.hpp>
#include <Akari.hpp>

namespace User {
	void IO::Puts(const char *s) {
		Akari->Console->PutString(s);
	}

	void IO::Putl(u32 n, u8 base) {
		Akari->Console->PutInt(n, base);
	}
}

