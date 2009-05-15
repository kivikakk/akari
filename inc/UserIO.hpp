#ifndef __USER_IO_HPP__
#define __USER_IO_HPP__

#include <arch.hpp>

namespace User {
	namespace IO {
		void Puts(const char *s);
		void Putl(u32 n, u8 base);
	}
}

#endif

