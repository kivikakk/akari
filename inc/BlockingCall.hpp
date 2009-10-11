#ifndef __BLOCKING_CALL_HPP__
#define __BLOCKING_CALL_HPP__

#include <arch.hpp>

class BlockingCall {
public:
	BlockingCall();
	virtual ~BlockingCall();

	bool shallBlock() const;

	virtual u32 operator ()() = 0;

protected:
	void _wontBlock();
	void _willBlock();

private:
	bool _shallBlock;
};

#endif

