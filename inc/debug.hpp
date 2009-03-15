#ifndef __DEBUG_HPP__
#define __DEBUG_HPP__

#include <Akari.hpp>

#ifdef DEBUG
#define ASSERT(x) 	AkariKernel::Assert(x)
#else
#define ASSERT(x)	
#endif

extern "C" void AkariPanic(const char *);

#endif

