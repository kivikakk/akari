#include <BlockingCall.hpp>

BlockingCall::BlockingCall(): _shallBlock(false) { }
BlockingCall::~BlockingCall() { }

bool BlockingCall::shallBlock() const {
	return _shallBlock;
}

void BlockingCall::_wontBlock() {
	_shallBlock = false;
}

void BlockingCall::_willBlock() {
	_shallBlock = true;
}

