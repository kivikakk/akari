#ifndef __DEBUG_HPP__
#define __DEBUG_HPP__

#ifdef DEBUG
#define ASSERT(x) 	Akari::Assert(x)
#else
#define ASSERT(x)	
#endif

#endif

