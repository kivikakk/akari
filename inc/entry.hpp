#ifndef __ENTRY_HPP__
#define __ENTRY_HPP__

extern "C" {
	extern void *AkariMultiboot;
	extern void *AkariStack;

	void AkariEntry();
}

#endif

