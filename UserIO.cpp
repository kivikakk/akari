#include <UserIO.hpp>
#include <Akari.hpp>

namespace User {
	void IO::Puts(const char *s) {
		Akari->Console->PutString(s);
	}
}

