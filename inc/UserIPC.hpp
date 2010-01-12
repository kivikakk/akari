#ifndef __USER_IPC_HPP__
#define __USER_IPC_HPP__

#include <arch.hpp>
#include <Tasks.hpp>

namespace User {
namespace IPC {
	bool registerName(const char *name);

	bool registerStream(const char *name);
	u32 obtainStreamWriter(const char *name, const char *node, bool exclusive);
	u32 obtainStreamListener(const char *name, const char *node);
	u32 readStream(const char *name, const char *node, u32 listener, char *buffer, u32 n);
	u32 readStreamUnblock(const char *name, const char *node, u32 listener, char *buffer, u32 n);
	u32 writeStream(const char *name, const char *node, u32 writer, const char *buffer, u32 n);

	bool probeQueue();
	bool probeQueueUnblock();
	u32 readQueue(char *dest, u32 offset, u32 len);
	void shiftQueue();
	void sendQueue(const char *name, u32 reply_to, const char *buffer, u32 len);
}
}

#endif

