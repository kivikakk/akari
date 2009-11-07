#ifndef __USER_CALLS_HPP__
#define __USER_CALLS_HPP__

#include <arch.hpp>
#include <Symbol.hpp>
#include <Tasks.hpp>
#include <BlockingCall.hpp>

namespace User {
	void putc(char c);
	void puts(const char *s);
	void putl(u32 n, u8 base);
	u32 getProcessId();
	void irqWait();
	void irqListen(u32 irq);
	void panic(const char *s);
	bool registerName(const char *name);
	bool registerStream(const char *name);
	bool registerQueue(const char *name);
	void exit();
	u32 obtainStreamWriter(const char *name, const char *node, bool exclusive);
	u32 obtainStreamListener(const char *name, const char *node);
	u32 readStream(const char *name, const char *node, u32 listener, char *buffer, u32 n);
	u32 readStreamUnblock(const char *name, const char *node, u32 listener, char *buffer, u32 n);
	u32 writeStream(const char *name, const char *node, u32 writer, const char *buffer, u32 n);
	void defer();
	void *malloc(u32 n);
	void free(void *p);
	void *memcpy(void *dest, const void *src, u32 n);

	class ReadCall : public BlockingCall {
	public:
		ReadCall(const char *name, const char *node, u32 listener, char *buffer, u32 n);

		Tasks::Task::Stream::Listener *getListener() const;

		u32 operator ()();

	protected:
		Tasks::Task::Stream::Listener *_listener;
		char *_buffer;
		u32 _n;
	};
}

#endif

