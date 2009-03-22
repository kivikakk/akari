#ifndef __DEBUG_HPP__
#define __DEBUG_HPP__

#ifdef DEBUG
#define ASSERT_STRINGIFY(x)	#x
#define ASSERT_TOSTRING(x)	ASSERT_STRINGIFY(x)
#define ASSERT(x) 	do { if (!(x)) { AkariPanic("Assertion at " __FILE__ ":" ASSERT_TOSTRING(__LINE__) " failed: " #x); } } while(0)
#else
#define ASSERT(x)	
#endif

extern "C" __attribute__((noreturn)) void AkariPanic(const char *);

#endif

